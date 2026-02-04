#!/usr/bin/bash

echo '#########################'
echo '# Rae vGamma Test Suite #'
echo '#########################'

execute_tst() {
	bin=$1
	code=$2
	desc=`echo $3 | sed -e 's/^[[:space:]]*//'`
	if [ ! -e $bin ]
	then
		echo "missing bin \"$bin\""
		exit 1
	fi
	$bin
	if [ "$?" -eq "$code" ]
	then
		echo "[  OK  ]: $desc"
	else
		echo "[FAILED]: $desc"
	fi
}

TST_DIR="./tst"
tests=`ls $TST_DIR/*.rae`
for tst in $tests
do
	bin=`echo $tst \
		| sed 's#tst#bin/tst#' \
		| sed 's/\.rae//'`
	desc=`cat $tst | head -n 1 | sed 's/#//'`
	execute_tst $bin 0 "$desc"
done
