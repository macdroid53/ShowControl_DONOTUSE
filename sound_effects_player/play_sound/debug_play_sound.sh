rm /tmp/*play_sound*.dot
GST_DEBUG_DUMP_DOT_DIR="/tmp" play_sound
rm *.dot
rm *.png
cp /tmp/*play_sound*.dot .
dot -Tpng -O *.dot
