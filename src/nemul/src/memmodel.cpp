//memmodel.cpp
//Timing model of the F1C200s memory system.
//
//Every constant here was measured on the real board by kernel/ports/f1c/bench
//(see that directory's README for the runs). Nothing is assumed: the numbers in
//this file are the ones the hardware produced, and the comment on each says which
//benchmark pinned it down.
//Bryan E. Topp <betopp@betopp.com> 2026

#include "memmodel.h"

#include <string.h>

#ifdef NEMUL_TIMING

//--------------------------------------------------------------------------------
//Measured constants
//--------------------------------------------------------------------------------

//Core clock. PLL_CPU is set to factor 25 from the 24MHz crystal. Confirmed twice
//on hardware: changing the PLL to factor 20 scaled every measured time by the
//same ratio, so this is the core clock and not an artefact of some fixed overhead.
static const uint64_t CLOCK_HZ = 600000000ull;

//Cache geometry: the datasheet says 32KByte I-cache and 16KByte D-cache, and the
//working-set sweeps put the knees at exactly 32KB and 16KB. 32-byte lines (the
//pointer chases step by one line and the knees are sharp), 4-way associative.
static const uint32_t LINE_BYTES = 32;
static const uint32_t ICACHE_BYTES = 32768;
static const uint32_t DCACHE_BYTES = 16384;
static const uint32_t WAYS = 4;

//Cost of filling one line from DRAM, in core cycles, excluding the eight
//one-cycle accesses that then consume it:
//  streaming read    101 cycles per line  -> row open, FILL_ROW_HIT  = 93
//  pointer chase     183 cycles per line  -> row closed, FILL_ROW_MISS = 175
//The ~82-cycle difference is a row activate plus precharge, which is why the
//model tracks open rows rather than applying one flat miss cost.
static const uint32_t FILL_ROW_HIT = 93;
static const uint32_t FILL_ROW_MISS = 175;

//Writing back a dirty line: streaming writes measured 63 cycles per line against
//101 for reads, because a full line goes out through the write buffer without
//reading the old contents first.
static const uint32_t WRITEBACK = 55;

//Display contention. Measured on the board by streaming reads with the display
//engine on and off: 640x480 cost 12.3% on a streaming read and 9.0% on a miss,
//320x240 cost 5.1% and 5.0%. The engine reads the framebuffer out of the same DDR,
//so it is modelled as a share of the bus: f = bytes scanned per second / the bus
//rate, and every fill is stretched by 1/(1-f). The bus rate is fitted to those two
//points and lands near the 300MByte/s measured for streaming writes.
static const double DRAM_BYTES_PER_SEC = 305.0e6;

//DDR1 geometry, from the controller's own SCONR value (0x0001B38A): 16-bit bus,
//10 column bits, 4 banks, 13 row bits. One row is 1024 columns * 2 bytes = 2KB.
static const uint32_t ROW_BYTES = 2048;
static const uint32_t BANKS = 4;

//--------------------------------------------------------------------------------
//Cache
//--------------------------------------------------------------------------------

struct cache_line_t
{
	uint32_t base;   //line address (MVA of byte 0), valid only if valid
	uint32_t used;   //age, for LRU
	uint8_t valid;
	uint8_t dirty;
	uint8_t written; //which of the eight words have been stored, bit per word
};

struct cache_t
{
	uint32_t sets;
	uint32_t index_shift; //LINE_BYTES == 1 << index_shift
	cache_line_t *line;   //sets * WAYS
	uint32_t age;
};

static cache_t icache;
static cache_t dcache;

static uint64_t cycles;
static uint32_t cur_pid = 0;
static uint32_t scanout_bytes = 0;
static mem_model_stats_t stats;

//Write buffer. The processor does not wait for a dirty line to reach DRAM: it hands
//the line to a small buffer that drains in the background, and only stalls when the
//buffer is full. Measured on the board by running the same store loop over a 64x64
//patch (fits in the cache, 13.20 cycles per pixel) and over the full 320x240 screen
//(does not fit, 13.09): no difference, so those writes cost nothing at all. The same
//comparison on a tight store loop gives 8.70 cached against 8.85 uncached, where the
//stores come faster than the buffer can drain and the CPU does stall. A model that
//charged every writeback to the CPU would be 40% pessimistic on the first case.
static void wb_write_back(void);

static const uint32_t WB_LINES = 4;
static uint64_t wb_done_at[WB_LINES];
static uint32_t wb_head = 0;
static uint32_t wb_count = 0;
static double bus_stretch = 1.0;

void mem_model_set_scanout(uint32_t bytes_per_frame)
{
	scanout_bytes = bytes_per_frame;
	if(bytes_per_frame == 0)
	{
		bus_stretch = 1.0;
		return;
	}

	double bytes_per_sec = (double)bytes_per_frame * 60.0;
	double share = bytes_per_sec / DRAM_BYTES_PER_SEC;
	if(share > 0.5)
		share = 0.5;
	bus_stretch = 1.0 / (1.0 - share);
}

//DRAM: which row is open in each bank (one row per bank can be open at a time)
static uint32_t open_row[BANKS];
static uint8_t open_valid[BANKS];

static cache_line_t icache_lines[(ICACHE_BYTES / LINE_BYTES / WAYS) * WAYS];
static cache_line_t dcache_lines[(DCACHE_BYTES / LINE_BYTES / WAYS) * WAYS];

//State for the load-use interlock: the destination register of the immediately
//preceding instruction if it was a load.
static uint32_t last_load_reg;
static int last_was_load;

static void cache_init(cache_t *c, uint32_t bytes, cache_line_t *storage, uint32_t index_shift)
{
	c->sets = bytes / LINE_BYTES / WAYS;
	c->index_shift = index_shift;
	c->line = storage;
	c->age = 1;
	uint32_t n = c->sets * WAYS;
	for(uint32_t i = 0; i < n; i++)
	{
		storage[i].base = 0;
		storage[i].used = 0;
		storage[i].valid = 0;
		storage[i].dirty = 0;
		storage[i].written = 0;
	}
}

//Virtual address to modified virtual address. Below the 32MByte FCSE window the
//kernel's view of a process is (pid << 25) | va, and so is the cache's: same
//line, same page table entry. Addresses above it are kernel addresses as they are.
static uint32_t to_mva(uint32_t va)
{
	if(va < 0x2000000u)
		return ((cur_pid & 0xFFu) << 25) | va;
	return va;
}

//Cost of bringing a line in from DRAM, tracking which row is open per bank.
static uint32_t dram_fill(uint32_t line_base)
{
	uint32_t row_addr = line_base / ROW_BYTES;
	uint32_t bank = row_addr % BANKS;
	uint32_t row = row_addr / BANKS;

	uint32_t cost = FILL_ROW_HIT;
	if(!open_valid[bank] || open_row[bank] != row)
	{
		cost = FILL_ROW_MISS;
		stats.row_misses++;
		open_row[bank] = row;
		open_valid[bank] = 1;
	}

	stats.fills++;

	//While the display engine is scanning, it holds part of the bus and this fill
	//takes proportionally longer.
	return (uint32_t)((double)cost * bus_stretch);
}

//Finds the line for this address, or NULL. Sets *way.
static cache_line_t *cache_find(cache_t *c, uint32_t mva, uint32_t *way_out)
{
	uint32_t set = (mva >> c->index_shift) % c->sets;
	uint32_t base = mva & ~(LINE_BYTES - 1u);
	cache_line_t *set_lines = &c->line[set * WAYS];
	for(uint32_t w = 0; w < WAYS; w++)
	{
		if(set_lines[w].valid && set_lines[w].base == base)
		{
			if(way_out)
				*way_out = w;
			return &set_lines[w];
		}
	}
	return 0;
}

//Picks a victim in this set (invalid first, then least recently used) and charges
//for evicting it if it is dirty.
static cache_line_t *cache_victim(cache_t *c, uint32_t mva)
{
	uint32_t set = (mva >> c->index_shift) % c->sets;
	cache_line_t *set_lines = &c->line[set * WAYS];

	uint32_t best = 0;
	for(uint32_t w = 0; w < WAYS; w++)
	{
		if(!set_lines[w].valid)
		{
			best = w;
			break;
		}
		if(set_lines[w].used < set_lines[best].used)
			best = w;
	}

	cache_line_t *victim = &set_lines[best];
	if(victim->valid && victim->dirty)
	{
		//A partially written line has to be read back before it can be written
		//out; a line the CPU filled completely does not, which is why streaming
		//writes (63 cycles per line) are cheaper than read-modify-write patterns.
		if(victim->written != 0xFFu)
			cycles += dram_fill(victim->base);
		stats.writebacks++;
		wb_write_back();
	}
	return victim;
}

static void fill_line(cache_t *c, uint32_t mva, cache_line_t *line)
{
	line->base = mva & ~(LINE_BYTES - 1u);
	line->valid = 1;
	line->dirty = 0;
	line->written = 0;
	line->used = c->age++;
}

//--------------------------------------------------------------------------------
//Public interface
//--------------------------------------------------------------------------------

void mem_model_reset(void)
{
	cache_init(&icache, ICACHE_BYTES, icache_lines, 5); //32-byte lines
	cache_init(&dcache, DCACHE_BYTES, dcache_lines, 5);
	cycles = 0;
	cur_pid = 0;
	for(uint32_t b = 0; b < BANKS; b++)
	{
		open_row[b] = 0;
		open_valid[b] = 0;
	}
	last_load_reg = 0x10;
	last_was_load = 0;
	scanout_bytes = 0;
	bus_stretch = 1.0;
	memset(&stats, 0, sizeof(stats));
	memset(wb_done_at, 0, sizeof(wb_done_at));
	wb_head = 0;
	wb_count = 0;
}

void mem_model_process(uint32_t pid)
{
	cur_pid = pid;
}

void mem_model_fetch(uint32_t va)
{
	uint32_t mva = to_mva(va);
	uint32_t way;
	//A hit costs nothing on top of the instruction's own issue cycle, which is
	//what makes a cached instruction one cycle. Only the fill is charged here.
	cache_line_t *line = cache_find(&icache, mva, &way);
	if(line)
	{
		line->used = icache.age++;
		return;
	}
	stats.icache_misses++;
	line = cache_victim(&icache, mva);
	fill_line(&icache, mva, line);
	//Fills use the same row model as data. That reproduces everything except a code
	//block that thrashes the I-cache, where the hardware charges about 162 cycles
	//per line and this charges about 101; see the note in the test. The rule that
	//would fix that case (charge lone fetches as row misses) overcharges ordinary
	//cached code by 23%, which is the case that actually happens, so it stays.
	cycles += dram_fill(mva);
}

void mem_model_load(uint32_t va, uint32_t bytes)
{
	(void)bytes;
	uint32_t mva = to_mva(va);
	uint32_t way;
	cache_line_t *line = cache_find(&dcache, mva, &way);
	if(line)
	{
		line->used = dcache.age++;
		return;
	}
	stats.dcache_read_misses++;
	line = cache_victim(&dcache, mva);
	fill_line(&dcache, mva, line);
	cycles += dram_fill(mva);
}

//Hands a dirty line to the write buffer. The CPU carries on unless the buffer is
//full, in which case it waits for the oldest line to finish draining: that is the
//difference between a program whose stores trickle out (costs nothing) and one that
//moves memory in bulk (costs the measured write bandwidth).
static void wb_write_back(void)
{
	uint32_t cost = (uint32_t)((double)WRITEBACK * bus_stretch);

	if(wb_count == WB_LINES)
	{
		uint32_t oldest = wb_head;
		if(wb_done_at[oldest] > cycles)
			cycles = wb_done_at[oldest];
		wb_head = (wb_head + 1u) % WB_LINES;
		wb_count--;
	}

	uint32_t slot = (wb_head + wb_count) % WB_LINES;
	uint64_t start = cycles;
	if(wb_count > 0)
	{
		uint32_t prev = (wb_head + wb_count - 1u) % WB_LINES;
		if(wb_done_at[prev] > start)
			start = wb_done_at[prev];
	}
	wb_done_at[slot] = start + cost;
	wb_count++;
}

void mem_model_store(uint32_t va, uint32_t bytes)
{
	uint32_t mva = to_mva(va);
	uint32_t way;
	cache_line_t *line = cache_find(&dcache, mva, &way);
	if(!line)
	{
		//Write-allocate without reading: the line is only read back if it is
		//evicted before every word has been written.
		stats.dcache_write_misses++;
		line = cache_victim(&dcache, mva);
		fill_line(&dcache, mva, line);
		line->dirty = 1;
	}
	line->used = dcache.age++;
	line->dirty = 1;

	//Record which words are written, for the partial-line eviction case. A
	//halfword or byte store marks the words it touches.
	uint32_t first = (mva & (LINE_BYTES - 1u)) / 4u;
	uint32_t count = (bytes + 3u) / 4u;
	for(uint32_t w = first; w < first + count && w < 8u; w++)
		line->written |= (uint8_t)(1u << w);
}

void mem_model_issue(uint32_t pc, uint32_t next_pc, uint32_t ir)
{
	//Classify. Everything not named here is a single-cycle operation, which is
	//what the hardware does: a 64-add loop measures 1.03 cycles per add whether
	//the adds are independent or form a dependency chain.
	uint32_t cost = 1;
	int is_load = 0;
	uint32_t load_rd = 0x10;
	uint32_t srcs = 0;

	uint32_t op2 = (ir >> 25) & 7;
	if(op2 == 5)
	{
		//B / BL
		cost = 1;
	}
	else if((ir & 0x0FFFFFF0u) == 0x012FFF10u)
	{
		//BX / BLX
		cost = 1;
	}
	else if((ir & 0x0FC000F0u) == 0x00000090u)
	{
		//MUL / MLA family: measured 2.75 cycles for back-to-back dependent muls.
		cost = 3;
	}
	else if(op2 == 4)
	{
		//LDM / STM: measured 4 cycles for four registers, i.e. one per register.
		uint32_t n = 0;
		for(uint32_t b = 0; b < 16u; b++)
			if(ir & (1u << b))
				n++;
		cost = n ? n : 1;
	}
	else if(op2 == 3)
	{
		//LDR / STR
		int load = (ir >> 20) & 1;
		if(load)
		{
			//Post-indexed and register-offset loads write the base back too; the
			//interlock only cares about the destination of the data.
			is_load = 1;
			load_rd = (ir >> 12) & 0xF;
		}
		cost = 1;
	}
	else
	{
		//Data processing / MSR / MRS
		cost = 1;
	}

	//Sources, for the load-use interlock: a data-processing instruction reads rn
	//and rm; a load or store reads its base and offset register.
	srcs = (1u << ((ir >> 16) & 0xF)) | (1u << ((ir >> 0) & 0xF));

	//A taken branch costs three extra cycles: subs plus a taken bne measured four
	//cycles in total, and a taken b alone measured four against one for the add
	//before it.
	if(next_pc != pc + 4u)
		cost += 3;

	//Load-use interlock: a load followed immediately by an instruction that reads
	//the loaded register costs one extra cycle (an ldr+add pair measured three
	//cycles where one plus one would fit; a stream of independent loads measures
	//one per load).
	if(last_was_load && (srcs & (1u << last_load_reg)))
		cost += 1;

	cycles += cost;

	last_was_load = is_load;
	last_load_reg = load_rd;
}

uint64_t mem_model_cycles(void)
{
	return cycles;
}

void mem_model_add_cycles(uint64_t c)
{
	cycles += c;
}

void mem_model_stats(struct mem_model_stats_t *out)
{
	*out = stats;
}

uint64_t mem_model_cycles_to_us(uint64_t c)
{
	return (c * 1000000ull) / CLOCK_HZ;
}

#endif //NEMUL_TIMING
