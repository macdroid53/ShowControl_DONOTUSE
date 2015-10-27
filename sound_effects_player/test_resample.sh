#!/bin/bash
rm /tmp/*.dot
export GST_PLUGIN_PATH=/usr/local/lib/gstreamer-1.0
export GST_DEBUG_FILE=gstreamer_trace.txt
export GST_DEBUG_DUMP_DOT_DIR="/tmp"
gst-launch-1.0 --gst-debug=*:5 -v -m filesrc location=background_music.wav ! wavparse ! audioconvert ! audio/x-raw,format=F32LE ! audioresample ! audio/x-raw,rate=96000 ! wavenc ! filesink location=output.wav
rm *.dot
rm *.png
cp /tmp/*.dot .
dot -Tpng -O *.dot

# end of file test_resample.sh


