/* user.ld
 * Linker script for userland
 * Bryan E. Topp <betopp@betopp.com> 2023
 */

ENTRY(_start)

SECTIONS {
	. = 0;
	
	_LINK_START = .;
	.flat :
	{
		/* NNE file header at the beginning */
		
		*(.header)
		KEEP(*(.header))
		KEEP(*(_header))
		
		/* Code sections */
		_TEXT_START = .;
		*(.text.first)
		*(.text*)
		
		
		/* lists of constructors and destructors */
		PROVIDE_HIDDEN ( __preinit_array_start = . );
		KEEP (*(.preinit_array))
		PROVIDE_HIDDEN ( __preinit_array_end = . );

		PROVIDE_HIDDEN ( __init_array_start = . );
		KEEP (*(SORT_BY_INIT_PRIORITY(.init_array.*) SORT_BY_INIT_PRIORITY(.ctors.*)))
		KEEP (*(.init_array .ctors))
		PROVIDE_HIDDEN ( __init_array_end = . );

		PROVIDE_HIDDEN ( __fini_array_start = . );
		KEEP (*(SORT_BY_INIT_PRIORITY(.fini_array.*) SORT_BY_INIT_PRIORITY(.dtors.*)))
		KEEP (*(.fini_array .dtors))
		PROVIDE_HIDDEN ( __fini_array_end = . );
		
		_TEXT_END = .;

		/* Read-only data including C++ exception nonsense */
		_RODATA_START = .;
		
		*(.note*)
		*(.rodata*)
		
		. = ALIGN(16);
		*(.gcc_except_table *.gcc_except_table.*)
		*(.ARM.extab* .gnu.linkonce.armextab.*)
	
		. = ALIGN(16);
		PROVIDE_HIDDEN ( __eh_frame_hdr_start = . );
		*(.eh_frame_hdr) *(.eh_frame_entry .eh_frame_entry.*)
		PROVIDE_HIDDEN ( __eh_frame_hdr_end = . );
	
		. = ALIGN(16);
		PROVIDE_HIDDEN ( __eh_frame_start = . );
		KEEP (*(.eh_frame .eh_frame.*))
		PROVIDE_HIDDEN ( __eh_frame_end = . );
	
		. = ALIGN(8);
		PROVIDE(__exidx_start = .);
		*(.ARM.exidx*)
		PROVIDE(__exidx_end = .);
		
		_RODATA_END = .;
		
		/* Mutable data */
		
		_DATA_START = .;
		*(.data*)
		_DATA_END = .;
		
		/* Thread-locals (just once as we are not multithreaded */
		_TLS_START = .;
		*(.tdata*)
		*(.tbss*)
		_TLS_END = .;
		_PVMK_LINKED_TP = .;
		
	}
	
	.bss : 
	{
		_BSS_START = .;
		*(COMMON)
		*(.bss*)
		_BSS_END = .;
	}
	
	_LINK_END = .;
	
	/* Globally defined on Neki32 - game process starts with all 24MBytes. */
	/* If you execute some other program be sure to append that many zeroes at the end. */
	_HEAP_END = 24*1024*1024;
	
	
	/* Stabs debugging sections.  */
	.stab          0 : { *(.stab) }
	.stabstr       0 : { *(.stabstr) }
	.stab.excl     0 : { *(.stab.excl) }
	.stab.exclstr  0 : { *(.stab.exclstr) }
	.stab.index    0 : { *(.stab.index) }
	.stab.indexstr 0 : { *(.stab.indexstr) }
	.comment       0 : { *(.comment) }
	.gnu.build.attributes : { *(.gnu.build.attributes .gnu.build.attributes.*) }
	/* DWARF debug sections.
	   Symbols in the DWARF debugging sections are relative to the beginning
	   of the section so we begin them at 0.  */
	/* DWARF 1.  */
	.debug          0 : { *(.debug) }
	.line           0 : { *(.line) }
	/* GNU DWARF 1 extensions.  */
	.debug_srcinfo  0 : { *(.debug_srcinfo) }
	.debug_sfnames  0 : { *(.debug_sfnames) }
	/* DWARF 1.1 and DWARF 2.  */
	.debug_aranges  0 : { *(.debug_aranges) }
	.debug_pubnames 0 : { *(.debug_pubnames) }
	/* DWARF 2.  */
	.debug_info     0 : { *(.debug_info .gnu.linkonce.wi.*) }
	.debug_abbrev   0 : { *(.debug_abbrev) }
	.debug_line     0 : { *(.debug_line .debug_line.* .debug_line_end) }
	.debug_frame    0 : { *(.debug_frame) }
	.debug_str      0 : { *(.debug_str) }
	.debug_loc      0 : { *(.debug_loc) }
	.debug_macinfo  0 : { *(.debug_macinfo) }
	/* SGI/MIPS DWARF 2 extensions.  */
	.debug_weaknames 0 : { *(.debug_weaknames) }
	.debug_funcnames 0 : { *(.debug_funcnames) }
	.debug_typenames 0 : { *(.debug_typenames) }
	.debug_varnames  0 : { *(.debug_varnames) }
	/* DWARF 3.  */
	.debug_pubtypes 0 : { *(.debug_pubtypes) }
	.debug_ranges   0 : { *(.debug_ranges) }
	/* DWARF 5.  */
	.debug_addr     0 : { *(.debug_addr) }
	.debug_line_str 0 : { *(.debug_line_str) }
	.debug_loclists 0 : { *(.debug_loclists) }
	.debug_macro    0 : { *(.debug_macro) }
	.debug_names    0 : { *(.debug_names) }
	.debug_rnglists 0 : { *(.debug_rnglists) }
	.debug_str_offsets 0 : { *(.debug_str_offsets) }
	.debug_sup      0 : { *(.debug_sup) }
	.gnu.attributes 0 : { KEEP (*(.gnu.attributes)) }
}
