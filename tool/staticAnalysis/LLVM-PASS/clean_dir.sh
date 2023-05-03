#! /bin/bash
for dir in `ls .`
 do
   if [ -d $dir ]
   then
     echo $dir
     cd $dir
     . clean.sh
     cd ..
   fi
done


echo "clean ${pwd}"
. clean.sh