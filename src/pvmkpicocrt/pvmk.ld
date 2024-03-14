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
		*(.init.i)
		*(.init)
		*(.init.n)
		*(.fini.i)
		*(.fini)
		*(.fini.n)
		_TEXT_END = .;

		_RODATA_START = .;
		*(.note*)
		*(.rodata*)
		*(.eh_frame*)
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
