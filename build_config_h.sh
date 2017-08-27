#!/bin/bash
# ---------------------------------------
# This shell builds config_html.h file
# used for the initial connect to an 
# wifi access point.
# Written by: Peter Schultz - hp@hpes.no
# ---------------------------------------
SRCDIR=main/config/html
DSTFILE=../config_html.h

cd $SRCDIR
cat > $DSTFILE << EOF
#ifndef __CONFIG_HTML__
#define __CONFIG_HTML__

// -----------------------------------------------
// Build time: `date`
// -----------------------------------------------
EOF

for f in *
do
  echo "Processing $f file..."
  echo "// --- Start $f ---" >> $DSTFILE
  xxd -i $f >> $DSTFILE
  echo "// --- End $f ---" >> $DSTFILE
done

cat >> $DSTFILE << EOF
#endif
EOF
