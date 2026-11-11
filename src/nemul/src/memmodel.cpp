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
static void wb_store(void);
static void wb_bus(uint32_t cost);

//Bus time one store occupies, measured directly: a loop storing one word with 0, 1, 2
//and 3 other instructions in it costs 8.97, 8.96, 8.79 and 8.72 cycles per store -
//flat, because the stores are what limit the loop - and only starts tracking the
//instruction count at 5 fillers (10.30 for 10 instructions) and 7 (12.09 for 12).
//So the drain is about 9 cycles per store and the buffer hides only about two stores
//of jitter. That also gives 9 cycles per halfword store, which is what the fill loops
//measure.
static const uint32_t STORE_BUS = 9;
//Slack the buffer can hide, in cycles of bus time. The store curve says a tight store
//loop is limited by the drain and a loop with ten cycles between stores is not, which
//is about two stores of slack. It also has to be at least one whole line writeback or
//a single eviction stalls the CPU, which is what the scroll benchmark caught: 55
//cycles of write-back against 18 of slack made every eviction stall and read 11% slow.
static const uint32_t WB_SLACK = 64;
static uint64_t wb_free_at = 0;
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

//State for the interlock: the destination register of the immediately preceding
//instruction if it produced one (load or multiply), and how many cycles a use of it
//costs.
static uint32_t last_result_reg;
static int last_was_result;
static uint32_t last_result_kind;


//History of the registers loads have produced, so a load whose address comes from an
//earlier load can be charged for it. Measured with a pointer chase carrying 0, 1, 2,
//3 and 5 filler instructions: 8.79, 9.77, 10.77, 11.78 and 13.77 cycles per hop, i.e.
//the fillers cost exactly one cycle each and a constant 3.8 cycles per hop sits on
//top of the instruction count however far apart the loads are. Charging four cycles
//when a load addresses itself from an earlier load reproduces that curve and also
//brings the cached pointer chase from 8 to 12.75 cycles per hop against 12.5 measured.
#define LOAD_HIST 8
static uint32_t load_hist_reg[LOAD_HIST];
static uint32_t load_hist_is[LOAD_HIST];
static uint32_t load_hist_pos;
static const uint32_t LOAD_ADDR_PENALTY = 4;

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
		//A dirty line evicted from the cache goes out through the DCache write-back
		//buffer, which is eight words behind a single address, so it costs the line
		//write and not the full fill. Only lines that were read in and then written
		//(write hits) reach here: stores that miss never allocate.
		//The evicted line goes out through the DCache write-back buffer, which is the
		//same buffered path as a store and not a synchronous stall. Charging it
		//directly made every read-then-write loop (scrolling, blitting) 18% slow.
		stats.writebacks++;
		wb_bus((uint32_t)((double)WRITEBACK * bus_stretch));
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
	last_result_reg = 0x10;
	last_was_result = 0;
	last_result_kind = 1;
	memset(load_hist_is, 0, sizeof(load_hist_is));
	load_hist_pos = 0;
	scanout_bytes = 0;
	bus_stretch = 1.0;
	memset(&stats, 0, sizeof(stats));
	wb_free_at = 0;
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
	{
		uint32_t f = dram_fill(mva);
		cycles += f;
		stats.fill_cycles += f;
	}
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
	{
		uint32_t f = dram_fill(mva);
		cycles += f;
		stats.fill_cycles += f;
	}
}

//A store that misses the cache goes into the write buffer and out to DRAM; it does
//not allocate a line. ARM926EJ-S TRM (r0p5, DDI0198D) table 4-4: for a write-back
//region a write hit goes to the DCache only, and a write miss is a "buffered store
//to external memory". There is no write-allocate, which is why a fill loop over a
//buffer that would fit in the cache costs the same as one over the whole screen.
//
//The buffer is 16 data words behind four addresses. Modelled as bus time: each store
//occupies the bus for STORE_BUS cycles, and the CPU only waits when the backlog
//exceeds what the buffer can hold. Measured consequence: a tight fill stalls and pays
//the full store cost (8.85 cycles per halfword store, 8 of which are bus), while a
//loop that stores once per 13 cycles pays nothing at all.
static void wb_bus(uint32_t cost)
{
	uint64_t start = (wb_free_at > cycles) ? wb_free_at : cycles;
	wb_free_at = start + cost;

	//Slack the buffer can hide, in cycles of bus time.
	if(wb_free_at > cycles + WB_SLACK)
	{
		stats.stall_cycles += (wb_free_at - WB_SLACK) - cycles;
		cycles = wb_free_at - WB_SLACK;
	}
}

static void wb_store(void)
{
	wb_bus((uint32_t)((double)STORE_BUS * bus_stretch));
}

void mem_model_store(uint32_t va, uint32_t bytes)
{
	uint32_t mva = to_mva(va);
	uint32_t way;
	(void)bytes;
	cache_line_t *line = cache_find(&dcache, mva, &way);
	if(!line)
	{
		//Write miss: buffered store, no line allocated (TRM table 4-4).
		stats.dcache_write_misses++;
		wb_store();
		return;
	}

	//Write hit: the line is in the cache, so the store costs only its issue cycle and
	//the line is dirtied for a later write-back.
	line->used = dcache.age++;
	line->dirty = 1;
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
	else if((ir & 0x0F8000F0u) == 0x00800090u)
	{
		//Long multiply: SMULL, UMULL, SMLAL, UMLAL. TRM table 8-2 gives 3 cycles, 4
		//when the following instruction uses the RdHi result. Software floating point
		//on this chip is built out of these, so they matter more than they look.
		cost = 3;
		is_load = 1;
		load_rd = (ir >> 16) & 0xF;
	}
	else if((ir & 0x0FC000F0u) == 0x00000090u)
	{
		//MUL / MLA: the TRM gives two cycles, and three when the following
		//instruction uses the result in its first operand (handled as the
		//interlock below, like a load).
		cost = 2;
		is_load = 1;
		load_rd = (ir >> 16) & 0xF;
	}
	else if(op2 == 4)
	{
		//LDM / STM: measured 4 cycles for four registers, i.e. one per register, which
		//is what the TRM gives (LDM n = n cycles). Loading the PC is the expensive case
		//and it is in every function return: TRM table 8-2 prices "LDM n+4" when the
		//list includes the PC, so `pop {r4, pc}` is six cycles, not two. Not charging
		//that made soft floating point, which is two library calls per iteration, read
		//13% fast.
		uint32_t n = 0;
		for(uint32_t b = 0; b < 16u; b++)
			if(ir & (1u << b))
				n++;
		if(!n)
			n = 1;
		int loads = (ir >> 20) & 1;
		if(loads && (ir & 0x8000u))
			cost = (n == 1) ? 5u : (n + 4u);
		else
			cost = n;
	}
	else if((ir & 0x0FBF0FFFu) == 0x010F0000u)
	{
		//MRS: two cycles (TRM table 8-2).
		cost = 2;
	}
	else if(op2 == 2)
	{
		//LDR / STR. Bits 27:25 = 010 for the single data transfer encoding: this is a
		//word load or store, and an earlier version of this model had the field as 011
		//here, which is the media encoding, so every real load fell through and was
		//charged as a plain ALU operation with no interlock at all.
		int load = (ir >> 20) & 1;
		if(load)
		{
			//Post-indexed and register-offset loads write the base back too; the
			//interlock only cares about the destination of the data. A byte or
			//halfword load used by the next instruction interlocks for two cycles
			//where a word load interlocks for one (TRM table 8-2), so the width
			//matters: bit 22 is the byte form, bits 6:5 select the halfword and
			//signed forms.
			int narrow = ((ir >> 22) & 1) || (((ir >> 5) & 3) != 0);
			is_load = narrow ? 2 : 1;
			load_rd = (ir >> 12) & 0xF;
		}

		//A scaled register offset costs an extra cycle (TRM table 8-2: "LDR 2 cycles,
		//not loading PC and shifted offset", likewise STR). The shift field is bits
		//11:7; bit 25 set means the offset is a register at all.
		if(((ir >> 25) & 1) && ((ir >> 7) & 0x1F))
			cost = 2;
		else
			cost = 1;
	}
	else if(op2 == 3 || (op2 == 0 && ((ir >> 4) & 9u) == 9u))
	{
		//Media: LDRH/STRH/LDRSB/LDRSH/LDRD/STRD/SWP. These hide in two encodings -
		//bits 27:25 = 011, and bits 27:25 = 000 with bits 7:4 = 1x11, which overlaps
		//the data-processing space. Missing the second form is how a halfword store
		//ended up being charged as a data operation with a register-controlled shift.
		//The multiply forms have bits 7:4 = 1001 and were taken above.
		int load = (ir >> 20) & 1;
		if(load && ((ir >> 6) & 1))
		{
			//A halfword or signed load: two-cycle interlock if its result is used
			//straight away.
			is_load = 2;
			load_rd = (ir >> 12) & 0xF;
		}
		cost = 1;
	}
	else
	{
		//Data processing / MSR / MRS. A shift whose amount comes from a register costs
		//a second cycle (TRM table 8-2: "Data Op 2 cycles, with register controlled
		//shift"). Software floating point normalises with those constantly, and the
		//model read 15% fast on it before this was charged.
		int reg_shift = ((ir >> 4) & 1) && !((ir >> 25) & 1);
		cost = reg_shift ? 2 : 1;
	}

	//Sources, for the interlock. The base register is always bits 19:16, but bits 3:0
	//are a register only when the instruction actually takes a register operand: in a
	//data-processing instruction bit 25 set means the operand is an immediate, and in a
	//load or store bit 25 set means the offset is a register. Reading the immediate as
	//a register invents a dependency on whichever register the constant happens to
	//name, which cost an extra cycle on every instruction with a small immediate.
	int rm_is_reg;
	if(op2 == 2 || op2 == 3)
		rm_is_reg = (ir >> 25) & 1;         //register offset form
	else if(op2 == 0 || op2 == 1)
		rm_is_reg = !((ir >> 25) & 1);      //immediate form has no rm
	else
		rm_is_reg = 0;

	srcs = (1u << ((ir >> 16) & 0xF));
	if(rm_is_reg)
		srcs |= (1u << ((ir >> 0) & 0xF));

	//A taken branch costs three extra cycles: subs plus a taken bne measured four
	//cycles in total, and a taken b alone measured four against one for the add
	//before it.
	//A taken branch costs three cycles in total, not three on top of its own issue
	//slot: TRM table 8-2 gives 3 cycles for B, BL, BX, BLX in all cases. The previous
	//version charged four, which made every short loop a cycle per iteration too slow.
	if(next_pc != pc + 4u)
	{
		stats.branch_cycles += 3;
		stats.taken_branches++;
		cost = 3;
	}

	//A load whose base register was produced by an earlier load pays the address
	//dependency: the data is not available in time to issue the next load, and the
	//measured cost of that is four cycles regardless of how many instructions separate
	//them. This replaces the one-cycle interlock for that case rather than adding to
	//it, since the measurement includes the interlock.
	int addr_from_load = 0;
	if(is_load && !(last_was_result && last_result_reg == ((ir >> 16) & 0xF)))
	{
		for(uint32_t h = 0; h < LOAD_HIST; h++)
		{
			if(load_hist_is[h] && load_hist_reg[h] == ((ir >> 16) & 0xF))
			{
				addr_from_load = 1;
				break;
			}
		}
	}
	if(addr_from_load)
	{
		cost += LOAD_ADDR_PENALTY;
		stats.interlock_cycles += LOAD_ADDR_PENALTY;
	}

	//Interlock: an instruction whose first operand is the result of the immediately
	//preceding load or multiply costs an extra cycle, two when the load was a byte or
	//halfword (TRM table 8-2: "LDR 2 cycles ... following instruction uses loaded
	//word", "3 cycles ... loaded byte, halfword or unaligned word used by following
	//instruction").
	else if(last_was_result && (srcs & (1u << last_result_reg)))
	{
		cost += last_result_kind;
		stats.interlock_cycles += last_result_kind;
	}
	//A distance-two dependency was tried here and rejected: it moved soft floating point
	//by one point and cost the screen gradient 7.7% and integer arithmetic 8.3%, because
	//those loops have a genuinely free slot between a load and its use. The evidence
	//does not support a general distance-two stall.

	cycles += cost;
	stats.instructions++;
	stats.issue_cycles += cost;

	//Remember what this instruction produced, for the address-dependency check above.
	load_hist_reg[load_hist_pos % LOAD_HIST] = load_rd;
	load_hist_is[load_hist_pos % LOAD_HIST] = (uint32_t)(is_load != 0);
	load_hist_pos++;

	last_was_result = is_load;
	last_result_reg = load_rd;
	//+1 for a word result (load or multiply), +2 for a byte or halfword load.
	last_result_kind = (is_load == 2) ? 2 : 1;
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
