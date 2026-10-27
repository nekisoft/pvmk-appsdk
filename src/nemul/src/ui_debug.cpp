//ui_debug.cpp
//Debugging window for the Neki32 simulator front end
//Bryan E. Topp <betopp@betopp.com> 2025

//Nemul, the Neki32 Simulator, Copyright 2025 Nekisoft Pty Ltd, ACN 680 583 251
//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

//What a debugger has to answer, in the order this window answers it: where is the machine
//stopped, what is it about to do, what did it just do, and how do I make it go again.

#include "ui_debug.h"
#include "disasm.h"

#include "imgui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//Names of the reasons a process can be held
static const char *const StopNames[PROCESS_DBGSTOP_MAX] =
{
	"", "interrupted", "signal", "breakpoint instruction", "data abort",
	"alignment check", "prefetch abort", "interpreter failure"
};

//The last part of a path, which is all that fits in a listing
static const char *base_name(const char *path)
{
	const char *slash = strrchr(path, '/');
	return (slash != NULL) ? (slash + 1) : path;
}

//Spells out an instruction with whatever the file can say about it
static void describe(ui_debug_t *st, uint32_t addr, uint32_t ir, bool annotate,
	char *out, size_t outsz, uint32_t *flow_out)
{
	char text[DISASM_MAX];
	uint32_t flow = (uint32_t)-1;
	if(disasm_arm(addr, ir, text, sizeof(text), &flow) < 0)
	{
		snprintf(text, sizeof(text), ".word	0x%8.8x", (unsigned)ir);
		flow = (uint32_t)-1;
	}
	if(flow_out != NULL)
		*flow_out = flow;

	if(!annotate)
	{
		snprintf(out, outsz, "%s", text);
		return;
	}

	//Where in the program this is, and which line of which file it came from
	char where[256] = "";
	if(st->syms != NULL)
	{
		uint32_t start = 0;
		const char *fn = symbols_func(st->syms, addr, &start, NULL);
		const char *file = NULL;
		int line = 0;
		const bool have_line = symbols_line(st->syms, addr, &file, &line);

		//Only the source position: the function is named above, and the room here is short
		(void)fn;
		(void)start;
		if(have_line)
			snprintf(where, sizeof(where), "%s:%d", base_name(file), line);
	}

	if(where[0] != 0)
		snprintf(out, outsz, "%-26s ; %s", text, where);
	else
		snprintf(out, outsz, "%s", text);
}

//One line of the disassembly: address, word, text
static void disasm_row(ui_debug_t *st, uint32_t addr, uint32_t ir, bool here)
{
	char text[512];
	describe(st, addr, ir, true, text, sizeof(text), NULL);

	//The encoded word is not shown: the text says what it does, and the room is needed for the
	//source position on the right
	(void)ir;
	if(here)
		ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "> %8.8x  %s", (unsigned)addr, text);
	else
		ImGui::Text("  %8.8x  %s", (unsigned)addr, text);
}

void ui_debug_open(ui_debug_t *st)
{
	st->open = true;
	st->follow_pc = true;
	st->show_trace = true;
	if(st->bp_text[0] == 0)
		snprintf(st->bp_text, sizeof(st->bp_text), "%s", "");
}

bool ui_debug_load_symbols(ui_debug_t *st, const char *path)
{
	if(st->syms != NULL)
	{
		symbols_free(st->syms);
		st->syms = NULL;
	}

	st->syms = symbols_load(path);
	if(st->syms == NULL)
	{
		snprintf(st->syms_status, sizeof(st->syms_status), "could not read %s", path);
		return false;
	}

	int funcs = 0, lines = 0;
	symbols_stats(st->syms, &funcs, &lines);
	snprintf(st->syms_status, sizeof(st->syms_status), "%d functions, %d source lines", funcs, lines);
	snprintf(st->syms_path, sizeof(st->syms_path), "%s", path);
	return true;
}

void ui_debug_close(ui_debug_t *st)
{
	if(st->syms != NULL)
	{
		symbols_free(st->syms);
		st->syms = NULL;
	}
	st->open = false;
}

//The process the window is looking at, or the only one there is
static const ui_debug_proc_t *pick_process(ui_debug_t *st, const ui_debug_state_t *state)
{
	if(st->pid != 0)
	{
		for(int pp = 0; pp < state->npids; pp++)
		{
			if(state->proc[pp].pid == st->pid)
				return &(state->proc[pp]);
		}
		//The process it was looking at has gone; fall back to whatever is there
		st->pid = 0;
	}

	if(state->npids == 0)
		return NULL;

	//Prefer the process the snapshot's code window was taken from
	for(int pp = 0; pp < state->npids; pp++)
	{
		if(state->proc[pp].pid == state->code_pid)
			return &(state->proc[pp]);
	}

	return &(state->proc[0]);
}

void ui_debug_draw(ui_debug_t *st, const ui_debug_state_t *state)
{
	if(!st->open)
		return;

	//Leave room for the window frame: the debugger is worth nearly the whole display
	const ImGuiViewport *viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + 24, viewport->WorkPos.y + 24),
		ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x - 48, viewport->WorkSize.y - 48),
		ImGuiCond_FirstUseEver);
	if(!ImGui::Begin("Debugger", &(st->open)))
	{
		ImGui::End();
		return;
	}

	//Requests are rebuilt every frame
	st->req.step = 0;
	st->req.pause = -1;
	st->req.bp_add = 0;
	st->req.bp_del = 0;
	st->req.bp_clear = 0;

	const ui_debug_proc_t *sel = pick_process(st, state);

	//The machine is stopped when the simulation is held, or when every process that exists is
	//held by a trap the debugger caught
	bool stopped = state->machine_paused;
	if(!stopped)
	{
		for(int pp = 0; pp < state->npids; pp++)
		{
			if(state->proc[pp].state == PROCESS_STATE_ALIVE && state->proc[pp].dbgstop == 0)
				stopped = false;
		}
	}

	//Controls
	if(ImGui::Button(stopped ? "Resume (F5)" : "Stop (F5)"))
		st->req.pause = stopped ? 0 : 1;
	ImGui::SameLine();
	ImGui::BeginDisabled(!stopped);
	if(ImGui::Button("Step"))
		st->req.step = 1;
	ImGui::SameLine();
	if(ImGui::Button("Step 10"))
		st->req.step = 10;
	ImGui::SameLine();
	if(ImGui::Button("Step 100"))
		st->req.step = 100;
	ImGui::EndDisabled();

	ImGui::SameLine();
	ImGui::Checkbox("Follow PC", &(st->follow_pc));
	ImGui::SameLine();
	ImGui::Checkbox("Trace", &(st->show_trace));

	ImGui::SameLine();
	ImGui::Text("  %s", stopped ? "stopped" : "running");
	ImGui::SameLine();
	ImGui::Text("   emulated %u ms", (unsigned)state->emul_ms);

	if(state->bp_hit_pid != 0 && state->machine_paused)
	{
		char text[128];
		uint32_t start = 0;
		const char *fn = (st->syms != NULL) ?
			symbols_func(st->syms, state->bp_hit_addr, &start, NULL) : NULL;
		snprintf(text, sizeof(text), "Stopped at breakpoint: pid %d at %8.8x%s%s",
			state->bp_hit_pid, (unsigned)state->bp_hit_addr,
			fn ? " in " : "", fn ? fn : "");
		ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.4f, 1.0f), "%s", text);
	}

	ImGui::Separator();

	//Left: the processes and the breakpoints
	ImGui::BeginChild("##procs", ImVec2(300, 0), ImGuiChildFlags_Borders);

	ImGui::Text("Processes");
	if(st->syms == NULL)
		ImGui::TextDisabled("(load the program's ELF for names)");

	if(ImGui::BeginTable("##proctable", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit))
	{
		ImGui::TableSetupColumn("pid");
		ImGui::TableSetupColumn("state");
		ImGui::TableSetupColumn("pc");
		ImGui::TableHeadersRow();

		for(int pp = 0; pp < state->npids; pp++)
		{
			const ui_debug_proc_t *proc = &(state->proc[pp]);
			ImGui::TableNextRow();
			ImGui::TableNextColumn();

			char label[64];
			snprintf(label, sizeof(label), "%d##pid%d", proc->pid, proc->pid);
			const bool selected = (sel != NULL && sel->pid == proc->pid);
			if(ImGui::Selectable(label, selected, ImGuiSelectableFlags_SpanAllColumns))
			{
				st->pid = proc->pid;
				st->follow_pc = true;
			}

			ImGui::TableNextColumn();
			const char *statetext = "?";
			if(proc->dbgstop != 0 && proc->dbgstop < PROCESS_DBGSTOP_MAX)
				statetext = StopNames[proc->dbgstop];
			else if(state->machine_paused && proc->state == PROCESS_STATE_ALIVE)
				statetext = "stopped";
			else if(proc->state == PROCESS_STATE_ALIVE && proc->paused)
				statetext = "blocked";
			else if(proc->state == PROCESS_STATE_ALIVE)
				statetext = "running";
			else if(proc->state == PROCESS_STATE_DEAD)
				statetext = "dead";
			ImGui::Text("%s", statetext);

			ImGui::TableNextColumn();
			uint32_t start = 0;
			const char *fn = (st->syms != NULL) ?
				symbols_func(st->syms, proc->pc, &start, NULL) : NULL;
			ImGui::Text("%8.8x %s", (unsigned)proc->pc, fn ? fn : "");
		}

		ImGui::EndTable();
	}

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Text("Breakpoints");

	for(int bb = 0; bb < state->bp_count; bb++)
	{
		char text[128];
		uint32_t start = 0;
		const char *fn = (st->syms != NULL) ?
			symbols_func(st->syms, state->bp[bb], &start, NULL) : NULL;
		snprintf(text, sizeof(text), "%8.8x  %s", (unsigned)state->bp[bb], fn ? fn : "");
		ImGui::Text("%s", text);

		ImGui::SameLine();
		char label[64];
		snprintf(label, sizeof(label), "remove##bp%d", bb);
		if(ImGui::SmallButton(label))
			st->req.bp_del = (int)state->bp[bb];
	}

	if(sel != NULL)
	{
		char label[64];
		snprintf(label, sizeof(label), "Break at PC (%8.8x)", (unsigned)sel->pc);
		if(ImGui::Button(label))
			st->req.bp_add = (int)sel->pc;
	}

	ImGui::SetNextItemWidth(90);
	if(ImGui::InputText("##address", st->bp_text, sizeof(st->bp_text),
		ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue))
	{
		const uint32_t addr = (uint32_t)strtoul(st->bp_text, NULL, 16);
		if(addr != 0)
			st->req.bp_add = (int)addr;
	}
	ImGui::SameLine();
	if(ImGui::Button("Add"))
	{
		const uint32_t addr = (uint32_t)strtoul(st->bp_text, NULL, 16);
		if(addr != 0)
			st->req.bp_add = (int)addr;
	}
	ImGui::SameLine();
	if(ImGui::Button("Clear all"))
		st->req.bp_clear = 1;

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Text("Symbols");
	ImGui::SetNextItemWidth(-70);
	ImGui::InputText("##sympath", st->syms_path, sizeof(st->syms_path));
	ImGui::SameLine();
	if(ImGui::Button("Load"))
	{
		if(st->syms_path[0] != 0)
			ui_debug_load_symbols(st, st->syms_path);
	}
	if(st->syms_status[0] != 0)
		ImGui::TextDisabled("%s", st->syms_status);

	ImGui::EndChild();

	ImGui::SameLine();

	//Right: registers and instructions
	ImGui::BeginChild("##view", ImVec2(0, 0), ImGuiChildFlags_Borders);

	if(sel == NULL)
	{
		ImGui::TextDisabled("No process is running.");
		ImGui::EndChild();
		ImGui::End();
		return;
	}

	//Registers
	ImGui::Text("Process %d", sel->pid);
	for(int rr = 0; rr < 16; rr += 4)
	{
		ImGui::Text(" %-3s %8.8x  %-3s %8.8x  %-3s %8.8x  %-3s %8.8x",
			disasm_regname(rr + 0), (unsigned)sel->regs[rr + 0],
			disasm_regname(rr + 1), (unsigned)sel->regs[rr + 1],
			disasm_regname(rr + 2), (unsigned)sel->regs[rr + 2],
			disasm_regname(rr + 3), (unsigned)sel->regs[rr + 3]);
	}

	//The status word, with the flags spelled out because nobody reads a mask in their head
	{
		const uint32_t cpsr = sel->cpsr;
		char flags[6] = "";
		int at = 0;
		if(cpsr & (1u << 31)) flags[at++] = 'N';
		if(cpsr & (1u << 30)) flags[at++] = 'Z';
		if(cpsr & (1u << 29)) flags[at++] = 'C';
		if(cpsr & (1u << 28)) flags[at++] = 'V';
		flags[at] = 0;

		//The interpreter runs user mode only, so the mode bits say nothing
		ImGui::Text("  cpsr %8.8x    flags %s    %s", (unsigned)cpsr,
			(flags[0] ? flags : "-"), (cpsr & (1u << 5)) ? "thumb" : "arm");
	}

	ImGui::Separator();

	//Where the program is: the function and line, if the ELF was loaded
	if(st->syms != NULL)
	{
		const char *file = NULL;
		int line = 0;
		uint32_t start = 0;
		const char *fn = symbols_func(st->syms, sel->pc, &start, NULL);
		const bool have_line = symbols_line(st->syms, sel->pc, &file, &line);
		ImGui::Text("%s:%d", have_line ? base_name(file) : "?", have_line ? line : 0);
		ImGui::Text("%s%s0x%x", fn ? fn : "?", fn ? "+" : "",
			(unsigned)(fn ? (sel->pc - start) : sel->pc));
	}

	//Disassembly, following the program counter or standing where it was put
	const uint32_t base = st->follow_pc ? (sel->pc - (UI_DEBUG_CODE_MAX / 2) * 4) : st->look_at;
	ImGui::BeginChild("##disasm", ImVec2(0, st->show_trace ? 260 : 0), ImGuiChildFlags_Borders);
	for(int ii = 0; ii < UI_DEBUG_CODE_MAX; ii++)
	{
		const uint32_t addr = base + (uint32_t)ii * 4;
		int idx = -1;
		if(state->code_count > 0 && addr >= state->code_base)
		{
			const uint32_t offset = addr - state->code_base;
			if(offset / 4 < (uint32_t)state->code_count)
				idx = (int)(offset / 4);
		}

		const uint32_t ir = (idx >= 0) ? state->code[idx] : 0;
		const bool here = (addr == sel->pc);

		if(here && st->follow_pc)
			ImGui::SetScrollHereY(0.5f);

		disasm_row(st, addr, ir, here);
	}
	ImGui::EndChild();

	if(st->show_trace)
	{
		ImGui::Text("Ran lately, newest first: pid %d, %d instructions held",
			state->trace_pid, state->trace_count);
		ImGui::BeginChild("##trace", ImVec2(0, 0), ImGuiChildFlags_Borders);
		for(int bb = 0; bb < state->trace_count; bb++)
		{
			//Entries run newest first, so the instruction that ran after this one is the entry
			//before it. Where a branch leads explains the shape of the run at a glance.
			char text[512];
			uint32_t flow = (uint32_t)-1;
			describe(st, state->trace_pc[bb], state->trace_ir[bb], false, text, sizeof(text), &flow);

			char went[48] = "";
			if(flow != (uint32_t)-1)
			{
				const uint32_t next = (bb > 0) ? state->trace_pc[bb - 1] : (uint32_t)-2;
				if(next == (uint32_t)-2)
					snprintf(went, sizeof(went), "  -> %8.8x", (unsigned)flow);
				else if(next == flow)
					snprintf(went, sizeof(went), "  -> %8.8x taken", (unsigned)flow);
				else
					snprintf(went, sizeof(went), "  -> %8.8x not taken", (unsigned)flow);
			}

			if(bb == 0)
				ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "%8.8x  %s%s",
					(unsigned)state->trace_pc[bb], text, went);
			else if(went[0] != 0)
				ImGui::TextColored(ImVec4(0.7f, 0.85f, 1.0f, 1.0f), "%8.8x  %s%s",
					(unsigned)state->trace_pc[bb], text, went);
			else
				ImGui::Text("%8.8x  %s", (unsigned)state->trace_pc[bb], text);
		}
		ImGui::EndChild();
	}

	ImGui::EndChild();
	ImGui::End();
}
