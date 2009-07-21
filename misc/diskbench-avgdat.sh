#!/bin/sh

get_write()
{
	x=`grep "# Average" $1 2>/dev/null | awk '{ print $8 }'`
	test -n "$x" || x=U
	test "$x" = "nan" && x=U
	echo "$x"
}

get_read()
{
	x=`grep "# Average" $1 2>/dev/null | awk '{ print $15 }'`
	test -n "$x" || x=U
	test "$x" = "nan" && x=U
	echo "$x"
}

echo "# bs	crx	wr	rd	wrx	rdx"
for mb in "$@"
do
	size=`echo $mb | sed -e 's/^0*//'`
	crx=`get_write $mb/*.crx.log`
	wr=`get_write $mb/*.wr.log`
	rd=`get_read $mb/*.wr.log`
	wrx=`get_write $mb/*.wrx.log`
	rdx=`get_read $mb/*.rdx.log`
	echo "$size	$crx	$wr	$rd	$wrx	$rdx"
done
