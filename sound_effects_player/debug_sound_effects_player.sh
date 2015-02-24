rm /tmp/*sound_effects_player*.dot
GST_DEBUG_DUMP_DOT_DIR="/tmp" sound_effects_player
rm *.dot
rm *.png
cp /tmp/*sound_effects_player*.dot .
dot -Tpng -O *.dot
