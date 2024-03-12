//crti.s
//Prologues for init/fini in ARM32 userland
//Bryan E. Topp <betopp@betopp.com> 2021


.section .init
.global _init
_init:
	push {fp, lr}
	b _init_body
	.word _init_epilogue
	.ltorg
	_init_body:

.section .fini
.global _fini
_fini:
	push {fp, lr}
	b _fini_body
	.word _fini_epilogue
	.ltorg
	_fini_body:
