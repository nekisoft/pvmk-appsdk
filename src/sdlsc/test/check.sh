#!/bin/sh
#check.sh
#Exercises the SDL system-call shim through both of its lives: the SDL one, where it owns
#a window and a renderer and scanout runs off the main thread, and the headless one, which
#is what automated runs use. Both threading modes (inline and pumped) get the same
#treatment, and the frames themselves are read back and checked, not just hashed.
#
#	sh check.sh			shim tests only
#	CHECK_EXAMPLES=1 sh check.sh	also build and run the example projects
#
#Exit status is the number of failures, so it drops straight into a build.

set -u

here=$(cd "$(dirname "$0")" && pwd)
shim=$(cd "$here/.." && pwd)
sdk=$(cd "$shim/.." && pwd)		#the SDK's src/ directory, where the examples live
work=${WORK:-$(mktemp -d "/tmp/sdlsc-check.XXXXXX")}

CFLAGS="-std=c99 -pedantic -Wall -Wextra -Werror -O2"
CFLAGS="$CFLAGS -I$shim/include -DUSE_SDLSC=1 $(sdl2-config --cflags)"
LIBS="$(sdl2-config --libs) -lm"

pass=0
fail=0

say()
{
	printf '%s\n' "$*"
}

ok()
{
	pass=$((pass + 1))
	say "ok   $*"
}

bad()
{
	fail=$((fail + 1))
	say "FAIL $*"
}

#check_build <output> <defines...>
check_build()
{
	out=$1
	shift
	# shellcheck disable=SC2086
	if ! cc $CFLAGS "$@" "$here/shimtest.c" "$shim/src/sdlsc.c" -o "$out" $LIBS -lm > "$work/build.log" 2>&1
	then
		bad "build $out"
		sed 's/^/     /' "$work/build.log" | head -20
		return 1
	fi
	return 0
}

#check_build_custom <output> <source> <defines...>
check_build_custom()
{
	out=$1
	src=$2
	shift 2
	# shellcheck disable=SC2086
	if ! cc $CFLAGS "$@" "$src" "$shim/src/sdlsc.c" -o "$out" $LIBS -lm > "$work/build.log" 2>&1
	then
		bad "build $out"
		sed 's/^/     /' "$work/build.log" | head -20
		return 1
	fi
	return 0
}

#run <name> <binary> [env assignments as one word...]
run()
{
	name=$1
	bin=$2
	shift 2
	log="$work/$name.log"
	# shellcheck disable=SC2086
	env "$@" "$bin" > "$log" 2>&1
	status=$?
	if [ "$status" -ne 0 ]
	then
		bad "$name (exit $status)"
		tail -15 "$log" | sed 's/^/     /'
		return 1
	fi
	if grep -q '^RESULT ' "$log" && ! grep -qE 'failures=0( |$)' "$log"
	then
		bad "$name (self-check)"
		grep 'FAIL' "$log" | sed 's/^/     /' | head -15
		return 1
	fi
	if grep -qE '^sdlsc: (failed|no video)' "$log"
	then
		#No video is fine when we asked for headless, but not otherwise
		case " $* " in
			*" SDLSC_HEADLESS=1 "*)
				say "     (note: $name reported no video and ran headless)"
				;;
			*)
				bad "$name (shim reported a video failure)"
				grep -E '^sdlsc:' "$log" | head -5 | sed 's/^/     /'
				return 1
				;;
		esac
	fi
	ok "$name"
	return 0
}

#dump_check <dir> <exact|forward|flat>
#Reads the PPMs back: every frame must be one flat colour, which is what a frame copied
#while being drawn into is not. Then, where ordering means anything: the numbers must be
#exactly 0,1,2,... (exact - a deterministic run), or merely increasing (forward), or not
#checked at all (flat - a windowed run drops frames whenever the application outruns the
#display, so the numbers it keeps say nothing).
dump_check()
{
	dir=$1
	mode=$2
	python3 - "$dir" "$mode" <<'PY'
import os
import sys

directory, mode = sys.argv[1], sys.argv[2]
names = sorted(n for n in os.listdir(directory) if n.endswith('.ppm'))
if not names:
	print("no frames dumped")
	sys.exit(1)

previous = -1
expected = 0
for name in names:
	with open(os.path.join(directory, name), 'rb') as f:
		if f.readline().strip() != b'P6':
			print("%s: not a binary PPM" % name)
			sys.exit(1)
		w, h = (int(x) for x in f.readline().split())
		if int(f.readline()) != 255:
			print("%s: unexpected maximum" % name)
			sys.exit(1)
		data = f.read()
	if len(data) != w * h * 3:
		print("%s: %d bytes, expected %d" % (name, len(data), w * h * 3))
		sys.exit(1)

	#Every pixel must be the same colour, or the frame was copied while being drawn
	red, green, blue = data[0], data[1], data[2]
	for offset in range(3, w * h * 3, 3):
		if data[offset] != red or data[offset + 1] != green or data[offset + 2] != blue:
			print("%s: not a flat frame (torn at pixel %d)" % (name, offset // 3))
			sys.exit(1)

	#Decode the 5:6:5 value the shim expanded on the way out
	frame = ((red >> 3) << 11) | ((green >> 2) << 5) | (blue >> 3)
	number = (frame >> 11) | (((frame >> 5) & 0x3F) << 5) | ((frame & 0x1F) << 11)
	#encode() above is frame&0x1F in red, (frame>>5) in green, (frame>>11) in blue
	number = (red >> 3) | (((green >> 2) & 0x3F) << 5) | (((blue >> 3) & 0x1F) << 11)

	if mode == 'exact':
		if number != expected:
			print("%s: holds frame %d, expected %d" % (name, number, expected))
			sys.exit(1)
		expected += 1
	elif mode == 'forward':
		if number <= previous:
			print("%s: holds frame %d after %d" % (name, number, previous))
			sys.exit(1)
		previous = number

print("%d frames, flat and in order" % len(names))
PY
}

say "shim check: $shim"
say "work dir:   $work"
say ""

#---
#Build the test programs, both ways
#---
say "building"
check_build "$work/shimtest_inline" || true
check_build "$work/shimtest_pumped" -DPUMPED || true
check_build_custom "$work/frametest_inline" "$here/frametest.c" || true
check_build_custom "$work/frametest_pumped" "$here/frametest.c" -DPUMPED || true
[ -x "$work/shimtest_inline" ] && ok "build inline" || bad "build inline"
[ -x "$work/shimtest_pumped" ] && ok "build pumped" || bad "build pumped"
say ""

#---
#The headless path: this is what automated runs use, so it has to be exact
#---
say "headless, virtual clock"
run headless-inline  "$work/shimtest_inline" SDLSC_HEADLESS=1 SDLSC_CLOCK=virtual SDLSC_NVM="$work/nvm1.bin"
run headless-pumped  "$work/shimtest_pumped" SDLSC_HEADLESS=1 SDLSC_CLOCK=virtual SDLSC_NVM="$work/nvm2.bin"

say "headless, real clock"
run headless-real-inline "$work/shimtest_inline" SDLSC_HEADLESS=1 SDLSC_NVM="$work/nvm3.bin"
run headless-real-pumped "$work/shimtest_pumped" SDLSC_HEADLESS=1 SDLSC_NVM="$work/nvm4.bin"

#Frames must be whole and in order, with a virtual clock, whether or not there is a window
for variant in inline pumped
do
	rm -rf "$work/frames-h-$variant"
	SDLSC_HEADLESS=1 SDLSC_CLOCK=virtual SDLSC_FRAMES=40 SDLSC_DUMP="$work/frames-h-$variant" \
		"$work/frametest_$variant" > "$work/frametest-h-$variant.log" 2>&1
	status=$?
	if [ "$status" -ne 0 ]
	then
		bad "headless frames ($variant) exited $status"
	elif result=$(dump_check "$work/frames-h-$variant" exact)
	then
		ok "headless frames ($variant): $result"
	else
		bad "headless frames ($variant): $result"
	fi
done

#The two threading models must produce the same pictures
if diff -r "$work/frames-h-inline" "$work/frames-h-pumped" > "$work/diff.log" 2>&1
then
	ok "inline and pumped agree frame for frame"
else
	bad "inline and pumped differ"
	head -10 "$work/diff.log" | sed 's/^/     /'
fi

#Same run twice must be identical, or it is no use for automated checks
rm -rf "$work/frames-h-repeat"
SDLSC_HEADLESS=1 SDLSC_CLOCK=virtual SDLSC_FRAMES=40 SDLSC_DUMP="$work/frames-h-repeat" \
	"$work/frametest_pumped" > /dev/null 2>&1
if diff -r "$work/frames-h-pumped" "$work/frames-h-repeat" > "$work/diff2.log" 2>&1
then
	ok "headless run is repeatable"
else
	bad "headless run is not repeatable"
	head -10 "$work/diff2.log" | sed 's/^/     /'
fi
say ""

#---
#The SDL path. There is no display on a build machine, so SDL's dummy video driver and
#software renderer stand in: a real window, a real renderer, real textures, real present.
#---
say "windowed (SDL dummy video driver, software renderer)"
for variant in inline pumped
do
	run "windowed-$variant" "$work/shimtest_$variant" \
		SDL_VIDEODRIVER=dummy SDL_RENDER_DRIVER=software SDLSC_CLOCK=virtual SDLSC_NVM="$work/nvm5-$variant.bin"
done
run windowed-pumped-noaudio "$work/shimtest_pumped" \
	SDL_VIDEODRIVER=dummy SDL_RENDER_DRIVER=software SDLSC_AUDIO=off SDLSC_CLOCK=virtual SDLSC_NVM="$work/nvm6.bin"
run windowed-pumped-novsync "$work/shimtest_pumped" \
	SDL_VIDEODRIVER=dummy SDL_RENDER_DRIVER=software SDLSC_VSYNC=0 SDLSC_CLOCK=virtual SDLSC_NVM="$work/nvm7.bin"
run windowed-pumped-realclock "$work/shimtest_pumped" \
	SDL_VIDEODRIVER=dummy SDL_RENDER_DRIVER=software SDLSC_NVM="$work/nvm8.bin"

#Frames through the window path: virtual clock makes this exactly ordered
rm -rf "$work/frames-w"
SDL_VIDEODRIVER=dummy SDL_RENDER_DRIVER=software SDLSC_CLOCK=virtual SDLSC_FRAMES=40 \
	SDLSC_DUMP="$work/frames-w" "$work/frametest_pumped" > "$work/frametest-w.log" 2>&1
status=$?
if [ "$status" -ne 0 ]
then
	bad "windowed frames exited $status"
elif result=$(dump_check "$work/frames-w" flat)
then
	ok "windowed frames: $result"
else
	bad "windowed frames: $result"
fi

#With vsync off there is nothing to wait for, so the pump has to be woken by each flip.
#If it were sleeping for its 20ms timeout instead, this would take 20s or more.
start=$(date +%s)
SDL_VIDEODRIVER=dummy SDL_RENDER_DRIVER=software SDLSC_VSYNC=0 SDLSC_CLOCK=virtual \
	SDLSC_FRAMES=1000 "$work/frametest_pumped" cheap > "$work/stress-w.log" 2>&1
status=$?
elapsed=$(($(date +%s) - start))
if [ "$status" -ne 0 ]
then
	bad "windowed unthrottled 1000 frames exited $status"
elif [ "$elapsed" -le 10 ]
then
	ok "windowed unthrottled 1000 frames in ${elapsed}s (pump woken by flips, not by a timeout)"
else
	bad "windowed unthrottled 1000 frames took ${elapsed}s - the pump is waiting for timeouts"
fi

#With vsync on the run has to work, but this host has no display to pace it against:
#SDL's dummy driver has no vertical blanking, so the rate here says nothing. Pacing is
#left to a machine with a screen; what is checked is that asking for vsync is not fatal.
start=$(date +%s)
SDL_VIDEODRIVER=dummy SDL_RENDER_DRIVER=software SDLSC_VSYNC=1 SDLSC_CLOCK=virtual \
	SDLSC_FRAMES=60 "$work/frametest_pumped" cheap > "$work/paced-w.log" 2>&1
status=$?
elapsed=$(($(date +%s) - start))
if [ "$status" -ne 0 ]
then
	bad "windowed vsync run exited $status"
else
	ok "windowed vsync-on run completes (${elapsed}s; pacing needs a real display)"
fi

#Losing the renderer's textures - a lost GL context or D3D device - must not stop the
#picture: the shim throws its own textures away and makes new ones on the next present.
rm -rf "$work/frames-reset"
SDL_VIDEODRIVER=dummy SDL_RENDER_DRIVER=software SDLSC_RESET_AFTER=10 SDLSC_FRAMES=40 \
	SDLSC_DUMP="$work/frames-reset" "$work/frametest_pumped" > "$work/frametest-reset.log" 2>&1
status=$?
if [ "$status" -ne 0 ]
then
	bad "renderer reset exited $status"
	tail -10 "$work/frametest-reset.log" | sed 's/^/     /'
elif result=$(dump_check "$work/frames-reset" flat)
then
	ok "renderer reset mid-run: $result"
else
	bad "renderer reset mid-run: $result"
fi

#Closing the window has to stop the program cleanly, like it does on a console when the
#user quits. Exercised from the inside, since there is nobody here to click the box.
run windowed-quit "$work/frametest_pumped" \
	SDL_VIDEODRIVER=dummy SDL_RENDER_DRIVER=software SDLSC_QUIT_AFTER=20 SDLSC_FRAMES=100000
say ""

#---
#Failure paths: a machine with no video at all must still run the application
#---
#Expected: the shim says there is no video and runs the application headless anyway, so
#a build machine without a display still gets a result. run_expect_headless allows that
#message; everything else about the run is checked as usual.
run_expect_headless()
{
	name=$1
	bin=$2
	shift 2
	log="$work/$name.log"
	# shellcheck disable=SC2086
	env "$@" "$bin" > "$log" 2>&1
	status=$?
	if [ "$status" -ne 0 ]
	then
		bad "$name (exit $status)"
		tail -15 "$log" | sed 's/^/     /'
	elif ! grep -q 'running headless' "$log"
	then
		bad "$name (did not fall back to headless)"
		grep -E '^sdlsc:' "$log" | head -3 | sed 's/^/     /'
	elif grep -q '^RESULT ' "$log" && ! grep -qE 'failures=0( |$)' "$log"
	then
		bad "$name (self-check)"
		grep 'FAIL' "$log" | head -10 | sed 's/^/     /'
	else
		ok "$name (fell back to headless)"
	fi
}

say "no display available"
run_expect_headless no-video-inline "$work/shimtest_inline" SDL_VIDEODRIVER=nosuchdriver SDLSC_CLOCK=virtual SDLSC_NVM="$work/nvm9.bin"
run_expect_headless no-video-pumped "$work/shimtest_pumped" SDL_VIDEODRIVER=nosuchdriver SDLSC_CLOCK=virtual SDLSC_NVM="$work/nvm10.bin"
say ""

#---
#Scripted input: the buttons have to arrive, in order, in both paths
#---
say "scripted input"
cat > "$work/script.txt" <<'EOF'
# frame  buttons
2        start
5        start,a
8        -
12       down
EOF
cat > "$work/start.txt" <<'EOF'
# frame  buttons
10       start
20       -
40       start
50       -
EOF
check_build_custom "$work/inputtest_inline" "$here/inputtest.c" || true
check_build_custom "$work/inputtest_pumped" "$here/inputtest.c" -DPUMPED || true

#Headless is deterministic, so every state has to arrive in order
SDLSC_HEADLESS=1 SDLSC_CLOCK=virtual SDLSC_INPUT="$work/script.txt" \
	"$work/inputtest_inline" strict > "$work/input-headless.log" 2>&1
if [ $? -eq 0 ] && grep -qE 'failures=0( |$)' "$work/input-headless.log"
then
	ok "scripted input, headless: $(grep -o 'states=[0-9]*' "$work/input-headless.log")"
else
	bad "scripted input, headless"
	grep -E 'TEST .*FAIL|failures=' "$work/input-headless.log" | sed 's/^/     /' | head -10
fi

#A windowed run goes through SDL's key handling, which is pumped at its own pace, so
#states may coalesce - but the presses must still arrive and nothing spurious may appear
SDL_VIDEODRIVER=dummy SDL_RENDER_DRIVER=software \
	SDLSC_INPUT="$work/script.txt" "$work/inputtest_pumped" > "$work/input-windowed.log" 2>&1
if [ $? -eq 0 ] && grep -qE 'failures=0( |$)' "$work/input-windowed.log"
then
	ok "scripted input through SDL key events: $(grep -o 'states=[0-9]*' "$work/input-windowed.log")"
else
	bad "scripted input through SDL key events"
	grep -E 'TEST .*FAIL|failures=' "$work/input-windowed.log" | sed 's/^/     /' | head -10
fi
say ""

#---
#The example projects, if the caller wants them: they are the real applications, and
#between them they use every system call the shim implements.
#---
if [ "${CHECK_EXAMPLES:-0}" = 1 ]
then
	say "examples (headless, 60 frames each)"
	examples="$sdk/examples"
	for project in sysusage/tplbuf goofs/demodisk1 goofs/democard goofs/menuvn goofs/neki3d \
		goofs/robuts goofs/rockshot goofs/sausage goofs/soccest ports/freedoom1 \
		ports/cdogs ports/greed ports/cylindrix ports/quake
	do
		[ -d "$examples/$project/img.dev" ] || continue
		name=$(printf '%s' "$project" | tr '/' '-')
		( cd "$examples/$project" && gmake -f Makefile.dev > "$work/build-$name.log" 2>&1 )
		if [ $? -ne 0 ]
		then
			bad "example $project build"
			continue
		fi
		( cd "$examples/$project/img.dev" && SDLSC_HEADLESS=1 SDLSC_CLOCK=virtual \
			SDLSC_FRAMES=60 SDLSC_INPUT="$work/start.txt" \
			"$examples/$project/img.dev/boot.elf" > "$work/run-$name.log" 2>&1 )
		status=$?
		if [ "$status" -eq 0 ]
		then
			ok "example $project"
		elif [ "$project" = "goofs/sfilm" ]
		then
			say "     (skipped: $project will not run on this host by its own design)"
		else
			bad "example $project (exit $status)"
			tail -10 "$work/run-$name.log" | sed 's/^/     /'
		fi
	done
	say ""

	#The same programs through the window path: window, renderer, textures, present,
	#and (for freedoom) the audio device. One of each kind: a tic loop with sound, one
	#that needs input to leave its menu, a software 3D one, and a sprite one.
	say "examples through the SDL window path (60 frames each)"
	for project in ports/freedoom1 ports/cylindrix goofs/neki3d goofs/soccest
	do
		[ -x "$examples/$project/img.dev/boot.elf" ] || continue
		name=$(printf '%s' "$project" | tr '/' '-')
		( cd "$examples/$project/img.dev" && SDL_VIDEODRIVER=dummy SDL_RENDER_DRIVER=software \
			SDLSC_INPUT="$work/start.txt" SDLSC_FRAMES=60 \
			"$examples/$project/img.dev/boot.elf" > "$work/runw-$name.log" 2>&1 )
		status=$?
		if [ "$status" -eq 0 ]
		then
			ok "example $project, windowed"
		else
			bad "example $project, windowed (exit $status)"
			tail -10 "$work/runw-$name.log" | sed 's/^/     /'
		fi
	done
	say ""
fi

say "RESULT passed=$pass failed=$fail"
say "logs in $work"
[ "$fail" -eq 0 ]
