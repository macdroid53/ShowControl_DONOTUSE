/*
 * button_subroutines.h
 *
 * Copyright © 2016 by John Sauter <John_Sauter@systemeyescomputerstore.com>
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
#ifndef BUTTON_SUBROUTINES_H
#define BUTTON_SUBROUTINES_H

#include <gtk/gtk.h>
#include <gst/gst.h>
#include "sound_structure.h"

/* Subroutines defined in button_subroutines.c */
void button_mute_toggled (GtkToggleButton * button, gpointer user_data);
void button_start_clicked (GtkButton * button, gpointer user_data);
void button_stop_clicked (GtkButton * button, gpointer user_data);
void button_set_cluster_playing (struct sound_info *sound_data,
                                 GApplication * app);
void button_reset_cluster (struct sound_info *sound_data, GApplication * app);
void button_volume_changed (GtkButton * button, gpointer user_data);
void button_pan_changed (GtkButton * button, gpointer user_data);

#endif /* BUTTON_SUBROUTINES_H */
/* End of file button_subroutines.h */
