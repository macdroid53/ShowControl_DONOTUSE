/*
 * button_clicked.h
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

/* Subroutines defined in button_clicked.c */
void start_clicked (GtkButton * button, gpointer user_data);
void stop_clicked (GtkButton * button, gpointer user_data);
void play_sound_volume_changed (GtkButton * button, gpointer user_data);
void play_sound_pan_changed (GtkButton * button, gpointer user_data);
