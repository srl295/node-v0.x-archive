#!/bin/sh
echo "Searching $1 to write out expfile to $2"
echo "#!." > $2
find $1 -name "*.a" |xargs nm -Xany -BCpg | awk '{ if ((($2 == "T") || ($2 == "D") || ($2 == "B")) && (substr($3,1,1) != ".")) {print $3} }'  | sort -u >> $2
