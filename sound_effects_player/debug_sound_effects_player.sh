rm /tmp/*sound_effects_player*.dot
GST_DEBUG_DUMP_DOT_DIR="/tmp" sound_effects_player ../The_Perils_of_Pauline/Pauline_project.xml
rm *.dot
rm *.png
cp /tmp/*sound_effects_player*.dot .
dot -Tpng -O *.dot
