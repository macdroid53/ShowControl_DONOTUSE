#!/bin/bash
rm /tmp/*.dot
export GST_PLUGIN_PATH=/usr/local/lib/gstreamer-1.0
export GST_DEBUG_FILE=gstreamer_trace.txt
export GST_DEBUG_DUMP_DOT_DIR="/tmp"
#gst-launch-1.0 --gst-debug=*:5 -v -m filesrc location=440Hz.wav ! wavparse ! audioconvert ! looper loop-from=1000000000 loop-limit=2 autostart=TRUE ! envelope autostart=TRUE attack-duration-time=1000000000 release-start-time=10000000000 release-duration-time=2000000000 ! audioconvert ! wavenc ! filesink location=output.wav
gst-launch-1.0 --gst-debug=*:5 -v -m filesrc location=440Hz.wav ! wavparse ! audioconvert ! envelope autostart=TRUE attack-duration-time=1000000000 release-start-time=6000000000 release-duration-time=2000000000 ! audioconvert ! wavenc ! filesink location=output.wav
rm *.dot
rm *.png
cp /tmp/*.dot .
dot -Tpng -O *.dot

# end of file test_looper.sh


