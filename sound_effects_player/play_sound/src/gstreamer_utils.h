/*
 * gstreamer_utils.h
 *
 * Copyright © 2015 by John Sauter <John_Sauter@systemeyescomputerstore.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include <gst/gst.h>

/* Subroutines defined in gstreamer_utils.c */
GstPipeline *setup_gstreamer (void);
void shutdown_gstreamer (GstPipeline * pipeline_element);
GstBin *play_sound_find_bin (GstPipeline * pipeline_element,
                             gchar * bin_name);
GstElement *play_sound_find_volume (GstBin * bin_element);
void play_sound_debug_dump_pipeline (GstPipeline * pipeline_element);
