#!/bin/bash
# ---------------------------------------
# This shell loads all files in main/html
# to the root of the fat flash partition 
# on the esp32 board.
# Written by: Peter Schultz - hp@hpes.no
# ---------------------------------------
SRCDIR=main/relay_gpio/html

if [ -z "$1" ]
  then
    echo "upload.sh <ip_adress>"
    exit;
fi
cd $SRCDIR
for f in *.*
do
  echo "Processing $f file..."
  curl -F "file=@$f;filename=$f" $1
done
