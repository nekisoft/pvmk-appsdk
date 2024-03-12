//crtn.s
//Epilogues for init/fini in ARM32 userland
//Bryan E. Topp <betopp@betopp.com> 2021

.section .init
.global _init_epilogue
_init_epilogue:
	pop {fp, lr}
	bx lr

.section .fini
.global _fini_epilogue
_fini_epilogue:
	pop {fp, lr}
	bx lr
