#!/bin/bash

#Purpose: perform $2 dns requests to test overhead of tagging
#$1 = number of trials


echo starting dns requests

for ((i=1;i<=$1;i++))
do
  echo "DNS test $i"
  total=0;
  for ((j=1;j<=$2;j++))
  do
    before=$(date '+%s%6N');
    dig @10.0.0.20 -p 53 www.test$j.com > /dev/null;
    after=$(date '+%s%6N');
    total=$((total+(after-before)));
    sleep $3;
  done
  echo "****** Total msec = $((total))";
done
