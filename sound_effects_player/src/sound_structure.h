/*
 * sound_structure.h
 *
 * Copyright © 2015 by John Sauter <John_Sauter@systemeyescomputerstore.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
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

/* Only define the structure once per source file.  */
#ifndef SOUND_STRUCTURE_H
#define SOUND_STRUCTURE_H

/* Define the structure which holds the definition of a sound.
 * This structure is shared between sound_effects_player, 
 * parse_xml_subroutines, button_subroutines and sound_subroutines. */

#include <gtk/gtk.h>
#include <gst/gst.h>

struct sound_info
{
  gchar *name;                  /* name of the sound */
  gchar *wav_file;              /* name of the file holding the waveform */
  gint attack_time;             /* attack time, in nanoseconds */
  gfloat attack_level;          /* 1.0 means 100% of volume */
  gint decay_time;              /* decay time, in nanoseconds */
  gint sustain_level;           /* 1.0 means 100% of volume */
  gint release_start_time;      /* release start time, in nanoseconds */
  gint release_duration_time;   /* release duration time, in nanoseconds */
  gboolean release_duration_time_infinite;      /* TRUE if duration is 
                                                   infinite */
  gint64 loop_from_time;        /* loop from time, in nanoseconds */
  gint64 loop_to_time;          /* loop to time, in nanoseconds */
  gint loop_limit;              /* loop limit, a count */
  gint64 start_time;            /* start time, in nanoseconds */
  gfloat designer_volume_level; /* 1.0 means 100% of waveform's volume */
  gfloat designer_pan;          /* -1.0 is left, 0.0 is center, 1.0 is right */
  gint MIDI_program_number;     /* MIDI program number, if specified */
  gboolean MIDI_program_number_specified;       /* TRUE if not empty */
  gint MIDI_note_number;        /* MIDI note number, if specified */
  gboolean MIDI_note_number_specified;  /* TRUE if not empty */
  gchar *OSC_name;              /* name used by OSC to activate */
  gboolean OSC_name_specified;  /* TRUE if not empty */
  gchar *function_key;          /* name of function key */
  gboolean function_key_specified;      /* TRUE if not empty */
  GtkWidget *cluster;           /* The cluster the sound is in, if any */
  GstBin *sound_control;        /* The Gstreamer bin for this sound effect */
  gint cluster_number;          /* The number of the cluster the sound is in */
};

#endif /* ifndef SOUND_STRUCTURE_H */
/* End of file sound_structure.h */
