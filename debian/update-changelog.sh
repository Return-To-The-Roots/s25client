#!/bin/bash

cd $(dirname $0)

OLDREV=$(head changelog -n 1 | cut -d '-' -f 2 | cut -d ')' -f 1)
if [ -z "$OLDREV" ] ; then
	OLDREV=0
fi
OLDREV=$((OLDREV+1))

HEAD=$(head ../.svn/entries -n 4 | tail -n 1)

msg=/tmp/msg.$$

echo "$OLDREV->$HEAD"
for (( r = $OLDREV; r <= $HEAD ; ++r ))
do
	echo "processing revision $r"

	LOG=$(cd .. && LANG=C svn2cl -a -i -r $r --authors=debian/authors.xml --break-before-msg=1 --stdout)
	if [ -z "$LOG" ] && [ ! $r = $HEAD ] ; then
		echo "skipped"
	else
		# parse date	
		D=$(LANG=C date --date="$(echo "$LOG" | head -n 1 | sed "s/\(.*\)  \(.*\)/\1/g")" +"%a, %d %b %Y %H:%M:%S %z")
		UD=$(LANG=C date --date="$(echo "$LOG" | head -n 1 | sed "s/\(.*\)  \(.*\)/\1/g")" +"%Y%m%d")
	
		echo "s25rttr ($UD-$r) hardy; urgency=low" > $msg
		
		# parse logmessage
		echo "" >> $msg
		if [ $r = $HEAD ] ; then
			echo "  * New upstream snapshot" >> $msg
		fi
	
		L=$(echo "$LOG" | tail -n +3 | sed "s/\t/  /g")
		if [ -z "$L"  ] ; then
			echo "  * [r$r] .:" >> $msg
			echo "    Empty log message" >> $msg
		else
			echo "$L" >> $msg
		fi
	
		echo "" >> $msg
		
		# parse author
		A=$(echo "$LOG" | head -n 1 | sed "s/\(.*\)  \(.*\)/\2/g")
		if [ -z "$A" ] ; then
			A="Return To The Roots Team <sf-team@siedler25.org>"
		fi
		
		# add author and date
		echo " -- $A  $D" >> $msg
		echo "" >> $msg
		
		mv changelog /tmp/changelog.$$
		cat $msg /tmp/changelog.$$ > changelog
	fi
done

rm -f $msg
rm -f /tmp/changelog.$$

php update-changelog.php
