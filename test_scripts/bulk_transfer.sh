#!/bin/bash

#Purpose: perform $1 downloads of test file

echo starting downloads

for ((i=1;i<=$1;i++))
do
  echo "Download $i";
  before=$(date '+%s%3N');
  curl http://10.0.0.20:8080/overhead.iso -o overhead.iso;

  after=$(date '+%s%3N');
  echo "****** Time = $((after - before))";
  sum=($(md5sum overhead.iso));
  echo "START MD5 diff";
  diff  <(echo "$sum" ) <(echo "d14cb9b6f48feda0563cda7b5335e4c0");
  echo "END MD5 diff";
  rm overhead.iso;
  sleep 5;
done
