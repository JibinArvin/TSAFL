#! /bin/bash

path=`pwd`
ls $path | while read line
do  
    if [ -d "$line" ];then
            cd $line
            echo $line
            ./cleanDIR.sh
            cd ..
    fi
done