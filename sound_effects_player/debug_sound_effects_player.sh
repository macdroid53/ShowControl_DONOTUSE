rm /tmp/*sound_effects_player*.dot
export GST_PLUGIN_PATH=/usr/local/lib/gstreamer-1.0
export GST_DEBUG_FILE=gstreamer_trace.txt
export GST_DEBUG_DUMP_DOT_DIR="/tmp"
sound_effects_player --gst-debug=*:5 --process-id-file sep.txt --monitor-file monitor.wav ../The_Perils_of_Pauline/Pauline_project.xml
#sound_effects_player --gst-debug=*:5 --process-id-file sep.txt ../The_Perils_of_Pauline/Pauline_project.xml
rm *.dot
rm *.png
cp /tmp/*sound_effects_player*.dot .
dot -Tpng -O *.dot
