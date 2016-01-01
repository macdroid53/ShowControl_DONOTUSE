/*
 * sequence_subroutines.h
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

#include <gtk/gtk.h>
#include <gst/gst.h>
#include "sequence_structure.h"
#include "sound_structure.h"

/* Subroutines defined in sequence_subroutines.c */

/* Initialize the internal sequencer */
void *sequence_init (GApplication * app);

/* Append a sequence item to the sequence.  */
void sequence_append_item (struct sequence_item_info *sequence_item_data,
                           GApplication * app);

/* Start the internal sequencer.  */
void sequence_start (GApplication * app);

/* Execute the MIDI Show Control command Go.  */
void sequence_MIDI_show_control_go (gchar * Q_number, GApplication * app);

/* Execute the MIDI Show Control command Go_off.  */
void sequence_MIDI_show_control_go_off (gchar * Q_number, GApplication * app);

/* Start the sound offered on a cluster.  */
void sequence_cluster_start (guint cluster_number, GApplication * app);

/* Stop the sound offered on a cluster.  */
void sequence_cluster_stop (guint cluster_number, GApplication * app);

/* Operator pushed the Play button.  */
void sequence_button_play (GApplication * app);

/* A sound has completed.  */
void sequence_sound_completion (struct sound_info *sound_effect,
                                GApplication * app);

/* A sound has been terminated.  */
void sequence_sound_termination (struct sound_info *sound_effect,
                                 GApplication * app);

/* End of file sequence_subroutines.h */
