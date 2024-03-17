/* user.ld
 * Linker script for userland
 * Bryan E. Topp <betopp@betopp.com> 2023
 */

ENTRY(_header)

SECTIONS {
	. = 0;
	
	_LINK_START = .;
	.flat :
	{
		*(.header)
		
		_TEXT_START = .;
		*(.text.first)
		*(.text*)
		_TEXT_END = .;

		_RODATA_START = .;
		
		*(.note*)
		*(.rodata*)
		*(.eh_frame*)
		
		/* Exception index tables (C++) */
		__exidx_start = .;
		*(.ARM.exidx*)
		__exidx_end = .;
		
		/* Exception handler tables (C++) */
		__extab_start = .;
		*(.ARM.extab*)
		__extab_end = .;
		
		/* Global constructors function-pointer array */
		__init_array_start = .;
		*(.init_array*)
		*(.ctors*)
		__init_array_end = .;
		
		/* Global destructors function-pointer array */
		__fini_array_start = .;
		*(.dtors*)
		*(.fini_array*)
		__fini_array_end = .;
		
		/* Validation functions function-pointer array */
		__preinit_array_start = .;
		*(.preinit_array*)
		__preinit_array_end = .;
		
		__ctors_start = .;
		*(.global_ctors*)
		__ctors_end = .;
		
		_RODATA_END = .;
		
		_DATA_START = .;
		*(.data*)
		_DATA_END = .;
	}
	
	.bss : 
	{
		_BSS_START = .;
		*(COMMON)
		*(.bss*)
		_BSS_END = .;
	}
	
	_LINK_END = .;
}
