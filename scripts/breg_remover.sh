#!/bin/bash

sideEffects=false

arg1=$1
arg2=$2

while getopts ":s" opt; do
    case $opt in
        s)
            sideEffects=true
            arg1=$2
            arg2=$3
            ;;
        \?)
            echo "invalid flag: -$OPTARG" >&2
            ;;
    esac
done


replacer="`dirname $0`/replacer.cfg"
remover="`dirname $0`/remover.cfg"

firstLineReplacer=`sed -n 1p $replacer`
firstLineRemover=`sed -n 1p $remover`

secondLineRemover=`sed -n 2p $remover`


if [ "$sideEffects" = true ] 
then

    if [[ "$secondLineRemover" = 'set side_effects'* ]]
    then
        sed -i "2s|.*|set side_effects true|" $remover
    fi
else
    if [[ "$secondLineRemover" = 'set side_effects'* ]]
    then
        sed -i "2s|.*|set side_effects false|" $remover
    fi
fi


if [[ "$firstLineReplacer" = 'set breg_file '* ]]
then
    sed -i "1s|.*|set breg_file $arg2|" $replacer
fi

if [[ "$firstLineRemover" = 'set breg_file '* ]]
then
    sed -i "1s|.*|set breg_file $arg2|" $remover
fi

./`dirname $0`/bde_verify -w --noovr --rewrite-dir=./ --config=$replacer -I/bb/build/share/stp/include/00offlonly -I`/home/hmaldona/bin/bdeverify-helper.pl -d source -v BBHDR`00depbuild -I`/home/hmaldona/bin/bdeverify-helper.pl -d source -v BBHDR`00deployed -I`/home/hmaldona/bin/bdeverify-helper.pl -d source -v BBHDR`00offlonly $arg1
./`dirname $0`/bde_verify -w --noovr --rewrite-dir=./ --config=$remover -I/bb/build/share/stp/include/00offlonly -I`/home/hmaldona/bin/bdeverify-helper.pl -d source -v BBHDR`00depbuild -I`/home/hmaldona/bin/bdeverify-helper.pl -d source -v BBHDR`00deployed -I`/home/hmaldona/bin/bdeverify-helper.pl -d source -v BBHDR`00offlonly $arg1\-rewritten


if [ ! -f $arg1\-rewritten\-rewritten ]
then
    mv $arg1\-rewritten $arg1\-rewritten.cpp  

else
    mv $arg1\-rewritten\-rewritten $arg1\-rewritten.cpp  
fi
