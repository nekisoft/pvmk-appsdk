set pagination off

set logging file dump.out
set logging overwrite off
set logging enabled off
set logging enabled on

set $dumpiter=0
while($dumpiter < 100000)
	printf "test_begin %d\n", $dumpiter
	info registers
	printf "ir 0x%8.8X ... ", ((unsigned int*)($pc))[0]
	stepi
	info registers
	set $dumpiter=$dumpiter+1
	printf "test_end\n"
end

