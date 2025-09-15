#!/usr/bin/env sh

TMPD=$(mktemp -d)
for HHA in 0 1 2 3 4 5 6 7
do
	for HHB in 0 1 2 3 4 5 6 7 8 9 A B C D E F
	do
		if [ ${HHA}${HHB} != 00 ]
		then
			echo 00 ${HHA}${HHB} | xxd -rg1  | magick -background black -fill white -font "Helvetica" -size 16x16 -pointsize 16 -gravity west label:@- ${TMPD}/${HHA}${HHB}.png 
		fi
	done
done
cp ${TMPD}/01.png ${TMPD}/00.png
ls ${TMPD}/*.png | awk '{print $0}END{print "font_small.png"}' | xargs magick convert -append
convert font_small.png font_small.xbm
rm ${TMPD}/*.png
rmdir ${TMPD}

