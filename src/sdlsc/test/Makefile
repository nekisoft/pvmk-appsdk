#Makefile for the SDL system-call shim's own tests
#Bryan E. Topp <betopp@betopp.com> 2024
#
#"gmake check" builds the test programs and runs the whole matrix: both threading modes,
#with and without a window, with scripted input, and with the frame dumps read back.
#check.sh does the building, so there is nothing to keep in step here.

check:
	sh check.sh

#Also run the example projects through the shim, which takes longer
check-all:
	CHECK_EXAMPLES=1 sh check.sh

clean:
	rm -f shimtest_inline shimtest_pumped frametest_inline frametest_pumped \
		inputtest_inline inputtest_pumped

.PHONY: check check-all clean
