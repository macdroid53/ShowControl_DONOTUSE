#!/bin/bash
rm /tmp/*.dot
export GST_PLUGIN_PATH=/usr/local/lib/gstreamer-1.0
export GST_DEBUG_FILE=gstreamer_trace.txt
export GST_DEBUG_DUMP_DOT_DIR="/tmp"
# test.wav is a one-second 440Hz sine wave.
gst-launch-1.0 --gst-debug=*:5 -v -m filesrc location=test.wav ! wavparse ! audioconvert ! audio/x-raw,format=F64LE ! envelope autostart=TRUE attack-duration-time=500000000 attack-level=1.0 decay-duration-time=200000000 sustain-level=0.8 release-start-time=800000000 release-duration-time=200000000 ! audioconvert ! audio/x-raw,format=S32LE ! wavenc ! filesink location=output.wav
rm *.dot
rm *.png
cp /tmp/*.dot .
dot -Tpng -O *.dot

# end of file test_envelope.sh


