#!/usr/bin/env sh

set -e

if [ "$#" -ne 2 ]; 
then
	echo "usage: genfont.sh <font name> <output prefix>" 
	exit 0
fi



TMPD=$(mktemp -d)
for HHA in 0 1 2 3 4 5 6 7
do
	for HHB in 0 1 2 3 4 5 6 7 8 9 A B C D E F
	do
		if [ ${HHA}${HHB} != 00 ]
		then
			echo 00 ${HHA}${HHB} | xxd -rg1  | magick -background black -fill white -font "$1" -size 32x32 -pointsize 32 -gravity west label:@- ${TMPD}/${HHA}${HHB}.png 
		fi
	done
done
cp ${TMPD}/01.png ${TMPD}/00.png
ls ${TMPD}/*.png | awk "{print \$0}END{print \"$2.png\"}" | xargs magick convert -append
convert $2.png $2.xbm
rm ${TMPD}/*.png
rmdir ${TMPD}

