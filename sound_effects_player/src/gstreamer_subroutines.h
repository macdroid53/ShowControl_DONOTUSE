/*
 * gstreamer_subroutines.h
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
#include <gtk/gtk.h>
#include <gst/gst.h>
#include "sound_structure.h"

/* Subroutines defined in gstreamer_subroutines.c */
GstPipeline *gstreamer_init (int sound_count, GApplication * app);
GstBin *gstreamer_create_bin (struct sound_info *sound_data, int sound_number,
                              GstPipeline * pipeline_element,
                              GApplication * app);
void gstreamer_complete_pipeline (GstPipeline * pipeline_element,
                                  GApplication * app);
void gstreamer_shutdown (GstPipeline * pipeline_element);
void gstreamer_set_proper_state (GApplication * app);
GstElement *gstreamer_get_volume (GstBin * bin_element);
GstElement *gstreamer_get_pan (GstBin * bin_element);
void gstreamer_dump_pipeline (GstPipeline * pipeline_element);
