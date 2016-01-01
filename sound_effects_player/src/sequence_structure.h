/*
 * sequence_structure.h
 *
 * Copyright © 2016 by John Sauter <John_Sauter@systemeyescomputerstore.com>
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
#ifndef SEQUENCE_STRUCTURE_H
#define SEQUENCE_STRUCTURE_H

/* Define the structure which holds the definition of a sequence item.
 * This structure is shared between 
 * parse_xml_subroutines and sequence_subroutines. */

#include <gtk/gtk.h>
#include <gst/gst.h>

/* There are several types of sequence item, as follows:  */
enum sequence_item_type
{ unknown, start_sound, stop, wait, offer_sound, cease_offering_sound,
  operator_wait, start_sequence
};

/* The following structure is used for all sequence items.  No item uses
 * all the fields.  */
struct sequence_item_info
{
  gchar *name;                  /* name of the sequence item */
  enum sequence_item_type type; /* The type of sequence item */
  gchar *sound_name;            /* The name of the sound to start */
  gchar *tag;                   /* The sound's or offering's tag, used to
                                 * stop the sound or offering.  */
  guint use_external_velocity;  /* If 1, use the velocity of an external
                                 * Note On message to scale the volume.  */
  gdouble volume;               /* Scale the sound designer's velocity by
                                 * this much when starting the sound.  */

  gdouble pan;                  /* Pan the sound by this much, in addition
                                 * to the specification in the sound,
                                 * when starting the sound.  */
  guint program_number;         /* The pgogram number, bank number and */
  guint bank_number;            /* cluster number are used to display */
  guint cluster_number;         /* a sound for the operator to control.  */
  gboolean cluster_number_specified;    /* If a cluster is not specified, 
                                         * one as chosen at run time.  */
  gchar *next_completion;       /* Sequence item to execute on completion.  */
  gchar *next_termination;      /* sequence item to execute on termination.  */
  gchar *next_starts;           /* sequence item to execute when this sound 
                                 * starts.  */
  guint importance;             /* importance of this sound, 
                                 * for display purposes.  */
  gchar *Q_number;              /* The Q_number is used by MIDI Show 
                                 * Control.  */
  gchar *text_to_display;       /* What to show the operator */
  gchar *next;                  /* The next sequence item to execute.  */
  guint64 time_to_wait;         /* Nanoseconds to wait in the Wait sequence 
                                 * item.  */
  gchar *next_to_start;         /* The sequence item to execute when
                                 * the operator presses the cluster's Start
                                 * button.  */
  gchar *next_play;             /* The sequence item to execute when
                                 * the operator presses the Play button.  */
  gint MIDI_program_number;     /* The MIDI program number and */
  gint MIDI_note_number;        /* note number which trigger this cluster */
  gboolean MIDI_note_number_specified;  /* They may be omitted.  */
  gchar *OSC_name;              /* The Open Show Control (OSC) name which 
                                 * triggers this cluster */
  guint macro_number;           /* Used by MIDI Show Control's Fire command
                                 * to trigger this cluster.  */
  gchar *function_key;          /* The function key which triggers this 
                                 * cluster */

};

#endif /* ifndef SEQUENCE_STRUCTURE_H */
/* End of file sequence_structure.h */
