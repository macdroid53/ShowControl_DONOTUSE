/*
 * parse_xml_subroutines.c
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

#include <stdlib.h>
#include <gtk/gtk.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include "parse_xml_subroutines.h"
#include "network_subroutines.h"
#include "sound_effects_player.h"
#include "sound_structure.h"
#include "sound_subroutines.h"
#include "sequence_structure.h"
#include "sequence_subroutines.h"

/* Dig through a sounds xml file, or the sounds content of an equipment
 * or project xml file, looking for the individual sounds.  Construct the
 * sound effect player's internal data structure for each sound.  */
static void
parse_sounds_info (xmlDocPtr sounds_file, gchar * sounds_file_name,
                   xmlNodePtr sounds_loc, GApplication * app)
{
  const xmlChar *name;
  xmlChar *name_data;
  gchar *file_dirname, *absolute_file_name;
  gdouble double_data;
  gint64 long_data;
  xmlNodePtr sound_loc;
  struct sound_info *sound_data;

  file_dirname = NULL;
  absolute_file_name = NULL;
  name_data = NULL;
  /* We start at the children of a "sounds" section.  Each child should
   * be a "version" or "sound" section. */
  while (sounds_loc != NULL)
    {
      name = sounds_loc->name;
      if (xmlStrEqual (name, (const xmlChar *) "version"))
        {
          name_data =
            xmlNodeListGetString (sounds_file, sounds_loc->xmlChildrenNode,
                                  1);
          if ((!g_str_has_prefix ((gchar *) name_data, (gchar *) "1.")))
            {
              g_printerr ("Version number of sounds is %s, "
                          "should start with 1.\n", name_data);
              return;
            }
        }
      if (xmlStrEqual (name, (const xmlChar *) "sound"))
        {
          /* This is a sound.  Copy its information.  */
          sound_loc = sounds_loc->xmlChildrenNode;
          /* Allocate a structure to hold sound information. */
          sound_data = g_malloc (sizeof (struct sound_info));
          /* Set the fields to their default values.  If a field does not
           * appear in the XML file, it will retain its default value.
           * This lets us add new fields without invalidating old XML files.
           */
          sound_data->name = NULL;
          sound_data->disabled = FALSE;
          sound_data->wav_file_name = NULL;
          sound_data->wav_file_name_full = NULL;
          sound_data->attack_duration_time = 0;
          sound_data->attack_level = 1.0;
          sound_data->decay_duration_time = 0;
          sound_data->sustain_level = 1.0;
          sound_data->release_start_time = 0;
          sound_data->release_duration_time = 0;
          sound_data->release_duration_infinite = FALSE;
          sound_data->loop_from_time = 0;
          sound_data->loop_to_time = 0;
          sound_data->loop_limit = 0;
          sound_data->max_duration_time = 0;
          sound_data->start_time = 0;
          sound_data->designer_volume_level = 1.0;
          sound_data->designer_pan = 0.0;
          sound_data->MIDI_program_number = 0;
          sound_data->MIDI_program_number_specified = FALSE;
          sound_data->MIDI_note_number = 0;
          sound_data->MIDI_note_number_specified = FALSE;
          sound_data->OSC_name = NULL;
          sound_data->OSC_name_specified = FALSE;
          sound_data->function_key = NULL;
          sound_data->function_key_specified = FALSE;

          /* These fields will be filled at run time.  */
          sound_data->sound_control = NULL;
          sound_data->cluster_widget = NULL;
          sound_data->cluster_number = 0;

          /* Collect information from the XML file.  */
          while (sound_loc != NULL)
            {
              name = sound_loc->name;
              if (xmlStrEqual (name, (const xmlChar *) "name"))
                {
                  /* This is the name of the sound.  It is mandatory.  */
                  name_data =
                    xmlNodeListGetString (sounds_file,
                                          sound_loc->xmlChildrenNode, 1);
                  sound_data->name = g_strdup ((gchar *) name_data);
                  xmlFree (name_data);
                  name_data = NULL;
                }
              if (xmlStrEqual (name, (const xmlChar *) "wav_file_name"))
                {
                  /* The name of the WAV file from which we take the
                   * waveform.  */
                  name_data =
                    xmlNodeListGetString (sounds_file,
                                          sound_loc->xmlChildrenNode, 1);
                  if (name_data != NULL)
                    {
                      sound_data->wav_file_name =
                        g_strdup ((gchar *) name_data);
                      xmlFree (name_data);
                      name_data = NULL;

                      /* If the file name does not have an absolute path,
                       * prepend the path of the sounds, equipment or project 
                       * file.  This allows wave files to be copied along with 
                       * the files that refer to them.  */
                      if (g_path_is_absolute (sound_data->wav_file_name))
                        {
                          g_free (absolute_file_name);
                          absolute_file_name =
                            g_strdup (sound_data->wav_file_name);
                        }
                      else
                        {
                          g_free (file_dirname);
                          file_dirname =
                            g_path_get_dirname (sounds_file_name);
                          g_free (absolute_file_name);
                          absolute_file_name =
                            g_build_filename (file_dirname,
                                              sound_data->wav_file_name,
                                              NULL);
                          g_free (file_dirname);
                          file_dirname = NULL;
                        }
                      sound_data->wav_file_name_full = absolute_file_name;
                      if (!g_file_test
                          (absolute_file_name, G_FILE_TEST_EXISTS))
                        {
                          g_printerr ("File %s does not exist.\n",
                                      absolute_file_name);
                          sound_data->disabled = TRUE;
                        }
                      absolute_file_name = NULL;
                    }
                }
              if (xmlStrEqual
                  (name, (const xmlChar *) "attack_duration_time"))
                {
                  /* The time required to ramp up the sound when it starts.  */
                  name_data =
                    xmlNodeListGetString (sounds_file,
                                          sound_loc->xmlChildrenNode, 1);
                  double_data = g_ascii_strtod ((gchar *) name_data, NULL);
                  xmlFree (name_data);
                  name_data = NULL;
                  sound_data->attack_duration_time = double_data * 1E9;
                }
              if (xmlStrEqual (name, (const xmlChar *) "attack_level"))
                {
                  /* The level we ramp up to.  */
                  name_data =
                    xmlNodeListGetString (sounds_file,
                                          sound_loc->xmlChildrenNode, 1);
                  if (name_data != NULL)
                    {
                      double_data =
                        g_ascii_strtod ((gchar *) name_data, NULL);
                      xmlFree (name_data);
                      name_data = NULL;
                      sound_data->attack_level = double_data;
                    }
                }
              if (xmlStrEqual (name, (const xmlChar *) "decay_duration_time"))
                {
                  /* Following the attack, the time to decrease the volume
                   * to the sustain level.  */
                  name_data =
                    xmlNodeListGetString (sounds_file,
                                          sound_loc->xmlChildrenNode, 1);
                  if (name_data != NULL)
                    {
                      double_data =
                        g_ascii_strtod ((gchar *) name_data, NULL);
                      xmlFree (name_data);
                      name_data = NULL;
                      sound_data->decay_duration_time = double_data * 1E9;
                    }
                }
              if (xmlStrEqual (name, (const xmlChar *) "sustain_level"))
                {
                  /* The volume to reach at the end of the decay.  */
                  name_data =
                    xmlNodeListGetString (sounds_file,
                                          sound_loc->xmlChildrenNode, 1);
                  if (name_data != NULL)
                    {
                      double_data =
                        g_ascii_strtod ((gchar *) name_data, NULL);
                      xmlFree (name_data);
                      name_data = NULL;
                      sound_data->sustain_level = double_data;
                    }
                }
              if (xmlStrEqual (name, (const xmlChar *) "release_start_time"))
                {
                  /* When to start the release process.  If this value is
                   * zero, we start the release process only upon receipt
                   * of an external signal, such as MIDI Note Off.  */
                  name_data =
                    xmlNodeListGetString (sounds_file,
                                          sound_loc->xmlChildrenNode, 1);
                  if (name_data != NULL)
                    {
                      double_data =
                        g_ascii_strtod ((gchar *) name_data, NULL);
                      xmlFree (name_data);
                      name_data = NULL;
                      sound_data->release_start_time = double_data * 1E9;
                    }
                }
              if (xmlStrEqual
                  (name, (const xmlChar *) "release_duration_time"))
                {
                  /* Once release has started, the time to ramp the volume
                   * down to zero.  Note this value may be infinity, which
                   * means that the volume does not decrease.  */
                  name_data =
                    xmlNodeListGetString (sounds_file,
                                          sound_loc->xmlChildrenNode, 1);
                  if (name_data != NULL)
                    {
                      if (xmlStrEqual (name_data, (const xmlChar *) "∞"))
                        {
                          sound_data->release_duration_infinite = TRUE;
                          sound_data->release_duration_time = 0;
                          xmlFree (name_data);
                          name_data = NULL;
                        }
                      else
                        {
                          double_data =
                            g_ascii_strtod ((gchar *) name_data, NULL);
                          xmlFree (name_data);
                          sound_data->release_duration_time =
                            double_data * 1E9;
                          sound_data->release_duration_infinite = FALSE;
                          name_data = NULL;
                        }
                    }
                }
              if (xmlStrEqual (name, (const xmlChar *) "loop_from_time"))
                {
                  /* If we are looping, the end time of the loop.  */
                  name_data =
                    xmlNodeListGetString (sounds_file,
                                          sound_loc->xmlChildrenNode, 1);
                  if (name_data != NULL)
                    {
                      double_data =
                        g_ascii_strtod ((gchar *) name_data, NULL);
                      xmlFree (name_data);
                      sound_data->loop_from_time = double_data * 1E9;
                      name_data = NULL;
                    }
                }
              if (xmlStrEqual (name, (const xmlChar *) "loop_to_time"))
                {
                  /* If we are looping, the start time of the loop.  
                   * Each time through the loop we play from start time
                   * to end time.  */
                  name_data =
                    xmlNodeListGetString (sounds_file,
                                          sound_loc->xmlChildrenNode, 1);
                  if (name_data != NULL)
                    {
                      double_data =
                        g_ascii_strtod ((gchar *) name_data, NULL);
                      xmlFree (name_data);
                      sound_data->loop_to_time = double_data * 1E9;
                      name_data = NULL;
                    }
                }
              if (xmlStrEqual (name, (const xmlChar *) "loop_limit"))
                {
                  /* The number of times to pass through the loop.  Zero
                   * means do not loop.  */
                  name_data =
                    xmlNodeListGetString (sounds_file,
                                          sound_loc->xmlChildrenNode, 1);
                  if (name_data != NULL)
                    {
                      long_data =
                        g_ascii_strtoll ((gchar *) name_data, NULL, 10);
                      xmlFree (name_data);
                      sound_data->loop_limit = long_data;
                      name_data = NULL;
                    }
                }
              if (xmlStrEqual (name, (const xmlChar *) "max_duration_time"))
                {
                  /* The maximum amount of time to absorb from the WAV file  */
                  name_data =
                    xmlNodeListGetString (sounds_file,
                                          sound_loc->xmlChildrenNode, 1);
                  if (name_data != NULL)
                    {
                      double_data =
                        g_ascii_strtod ((gchar *) name_data, NULL);
                      xmlFree (name_data);
                      sound_data->max_duration_time = double_data * 1E9;
                      name_data = NULL;
                    }
                }
              if (xmlStrEqual (name, (const xmlChar *) "start_time"))
                {
                  /* The time within the WAV file to start this sound effect.
                   */
                  name_data =
                    xmlNodeListGetString (sounds_file,
                                          sound_loc->xmlChildrenNode, 1);
                  if (name_data != NULL)
                    {
                      double_data =
                        g_ascii_strtod ((gchar *) name_data, NULL);
                      xmlFree (name_data);
                      sound_data->start_time = double_data * 1E9;
                      name_data = NULL;
                    }
                }

              if (xmlStrEqual
                  (name, (const xmlChar *) "designer_volume_level"))
                {
                  /* For this sound effect, decrease the volume from the WAV
                   * file by this amount.  */
                  name_data =
                    xmlNodeListGetString (sounds_file,
                                          sound_loc->xmlChildrenNode, 1);
                  if (name_data != NULL)
                    {
                      double_data =
                        g_ascii_strtod ((gchar *) name_data, NULL);
                      xmlFree (name_data);
                      sound_data->designer_volume_level = double_data;
                      name_data = NULL;
                    }
                }

              if (xmlStrEqual (name, (const xmlChar *) "designer_pan"))
                {
                  /* For monaural WAV files, the amount to send to the left and
                   * right channels, expressed as -1 for left channel only,
                   * 0 for both channels equally, and +1 for right channel
                   * only.  Other values between +1 and -1 also place the sound
                   * in the stereo field.  For stereo WAV files this operates
                   * as a balance control.  */
                  name_data =
                    xmlNodeListGetString (sounds_file,
                                          sound_loc->xmlChildrenNode, 1);
                  if (name_data != NULL)
                    {
                      double_data =
                        g_ascii_strtod ((gchar *) name_data, NULL);
                      xmlFree (name_data);
                      sound_data->designer_pan = double_data;
                      name_data = NULL;
                    }
                }

              if (xmlStrEqual (name, (const xmlChar *) "MIDI_program_number"))
                {
                  /* If we aren't using the internal sequencer, the MIDI 
                   * program number within which a MIDI Note On will activate 
                   * this sound effect.  */
                  name_data =
                    xmlNodeListGetString (sounds_file,
                                          sound_loc->xmlChildrenNode, 1);
                  if (name_data != NULL)
                    {
                      long_data =
                        g_ascii_strtoll ((gchar *) name_data, NULL, 10);
                      xmlFree (name_data);
                      sound_data->MIDI_program_number = long_data;
                      sound_data->MIDI_program_number_specified = TRUE;
                    }
                  name_data = NULL;
                }

              if (xmlStrEqual (name, (const xmlChar *) "MIDI_note_number"))
                {
                  /* If we aren't using the internal sequencer, the MIDI Note
                   * number that will activate this sound effect.  */
                  name_data =
                    xmlNodeListGetString (sounds_file,
                                          sound_loc->xmlChildrenNode, 1);
                  if (name_data != NULL)
                    {
                      long_data =
                        g_ascii_strtoll ((gchar *) name_data, NULL, 10);
                      xmlFree (name_data);
                      sound_data->MIDI_note_number = long_data;
                      sound_data->MIDI_note_number_specified = TRUE;
                      name_data = NULL;
                    }
                }

              if (xmlStrEqual (name, (const xmlChar *) "OSC_name"))
                {
                  /* If we are not using the internal sequencer, this is the
                   * name by which this sound effect is activated using
                   * Open Sound Control.  */
                  name_data =
                    xmlNodeListGetString (sounds_file,
                                          sound_loc->xmlChildrenNode, 1);
                  if (name_data != NULL)
                    {
                      sound_data->OSC_name = g_strdup ((gchar *) name_data);
                      sound_data->OSC_name_specified = TRUE;
                      xmlFree (name_data);
                      name_data = NULL;
                    }
                }

              if (xmlStrEqual (name, (const xmlChar *) "function_key"))
                {
                  /* If we are not using the internal sequencer, this is the
                   * function key the operator presses to activate this
                   * sound effect.  */
                  name_data =
                    xmlNodeListGetString (sounds_file,
                                          sound_loc->xmlChildrenNode, 1);
                  if (name_data != NULL)
                    {
                      sound_data->function_key =
                        g_strdup ((gchar *) name_data);
                      sound_data->function_key_specified = TRUE;
                      xmlFree (name_data);
                      name_data = NULL;
                    }
                }

              /* Ignore fields we don't recognize, so we can read future
               * XML files. */

              sound_loc = sound_loc->next;
            }
          /* Append this sound to the list of sounds.  */
          sound_append_sound (sound_data, app);
        }
      sounds_loc = sounds_loc->next;
    }

  return;
}

/* Dig through a sequence xml file, or the sequence content of an equipment
 * or project xml file, looking for the individual sequence items.  
 * Construct the sound effect player's internal data structure for each 
 * sequence item.  */
static void
parse_sequence_info (xmlDocPtr sequence_file, gchar * sequence_file_name,
                     xmlNodePtr sequence_loc, GApplication * app)
{
  const xmlChar *name;
  xmlChar *name_data;
  gdouble double_data;
  gint64 long_data;
  xmlNodePtr sequence_item_loc;
  struct sequence_item_info *sequence_item_data;
  enum sequence_item_type item_type;

  name_data = NULL;
  /* We start at the children of a "sequence" section.  Each child should
   * be a "version" or "sequence_item" section. */
  while (sequence_loc != NULL)
    {
      name = sequence_loc->name;
      if (xmlStrEqual (name, (const xmlChar *) "version"))
        {
          name_data =
            xmlNodeListGetString (sequence_file,
                                  sequence_loc->xmlChildrenNode, 1);
          if ((!g_str_has_prefix ((gchar *) name_data, (gchar *) "1.")))
            {
              g_printerr ("Version number of sequence is %s, "
                          "should start with 1.\n", name_data);
              return;
            }
        }
      if (xmlStrEqual (name, (const xmlChar *) "sequence_item"))
        {
          /* This is a sequence item.  Copy its information.  */
          sequence_item_loc = sequence_loc->xmlChildrenNode;
          /* Allocate a structure to hold sequence item information. */
          sequence_item_data = g_malloc (sizeof (struct sequence_item_info));
          /* Set the fields to their default values.  If a field does not
           * appear in the XML file, it will retain its default value.
           * This lets us add new fields without invalidating old XML files.
           */
          /* Fields used in the Start Sound sequence item.  */
          sequence_item_data->name = NULL;
          sequence_item_data->type = unknown;
          sequence_item_data->sound_name = NULL;
          sequence_item_data->tag = NULL;
          sequence_item_data->use_external_velocity = 0;
          sequence_item_data->volume = 1.0;
          sequence_item_data->pan = 0.0;
          sequence_item_data->program_number = 0;
          sequence_item_data->bank_number = 0;
          sequence_item_data->cluster_number = 0;
          sequence_item_data->cluster_number_specified = FALSE;
          sequence_item_data->next_completion = NULL;
          sequence_item_data->next_terminated = NULL;
          sequence_item_data->next_starts = NULL;
          sequence_item_data->importance = 0;
          sequence_item_data->Q_number = NULL;
          sequence_item_data->text_to_display = NULL;

          /* Fields used in the Stop sequence item but not mentioned above.  */
          sequence_item_data->next = NULL;

          /* Fields used in the Wait sequence item but not mentioned above.  */
          sequence_item_data->time_to_wait = 0;

          /* Fields used in the Offer Sound sequence item but not mentioned
           * above.  */
          sequence_item_data->next_to_start = NULL;
          sequence_item_data->MIDI_program_number = 0;
          sequence_item_data->MIDI_note_number = 0;
          sequence_item_data->MIDI_note_number_specified = FALSE;
          sequence_item_data->OSC_name = NULL;
          sequence_item_data->macro_number = 0;
          sequence_item_data->function_key = NULL;

          /* The Cease Offering Sounds, Operator Wait and Start Sequence
           *  sequence items uses only fields already mentioned.  */

          /* Collect information from the XML file.  */
          while (sequence_item_loc != NULL)
            {
              name = sequence_item_loc->name;
              if (xmlStrEqual (name, (const xmlChar *) "name"))
                {
                  /* This is the name of the sequence item.  It is mandatory.  
                   */
                  name_data =
                    xmlNodeListGetString (sequence_file,
                                          sequence_item_loc->xmlChildrenNode,
                                          1);
                  sequence_item_data->name = g_strdup ((gchar *) name_data);
                  xmlFree (name_data);
                  name_data = NULL;
                }

              if (xmlStrEqual (name, (const xmlChar *) "type"))
                {
                  /* The type field specifies what this sequence item does.  */
                  name_data =
                    xmlNodeListGetString (sequence_file,
                                          sequence_item_loc->xmlChildrenNode,
                                          1);

                  /* Convert the textual name in the XML file into an enum.  */
                  item_type = unknown;
                  if (xmlStrEqual
                      (name_data, (const xmlChar *) "start_sound"))
                    {
                      item_type = start_sound;
                    }
                  if (xmlStrEqual (name_data, (const xmlChar *) "stop"))
                    {
                      item_type = stop;
                    }
                  if (xmlStrEqual (name_data, (const xmlChar *) "wait"))
                    {
                      item_type = wait;
                    }
                  if (xmlStrEqual
                      (name_data, (const xmlChar *) "offer_sound"))
                    {
                      item_type = offer_sound;
                    }
                  if (xmlStrEqual
                      (name_data, (const xmlChar *) "cease_offering_sound"))
                    {
                      item_type = cease_offering_sound;
                    }
                  if (xmlStrEqual
                      (name_data, (const xmlChar *) "operator_wait"))
                    {
                      item_type = operator_wait;
                    }
                  if (xmlStrEqual
                      (name_data, (const xmlChar *) "start_sequence"))
                    {
                      item_type = start_sequence;
                    }

                  sequence_item_data->type = item_type;
                  xmlFree (name_data);
                  name_data = NULL;
                }

              if (xmlStrEqual (name, (const xmlChar *) "sound_name"))
                {
                  /* For the Start Sound sequence item, the name of the sound
                   * to start.  */
                  name_data =
                    xmlNodeListGetString (sequence_file,
                                          sequence_item_loc->xmlChildrenNode,
                                          1);
                  sequence_item_data->sound_name =
                    g_strdup ((gchar *) name_data);
                  xmlFree (name_data);
                  name_data = NULL;
                }

              if (xmlStrEqual (name, (const xmlChar *) "tag"))
                {
                  /* The tag in Start Sound and Offer Sound is used by Stop
                   * and Cease Offering Sound to name the sound or offering
                   * to stop.  */
                  name_data =
                    xmlNodeListGetString (sequence_file,
                                          sequence_item_loc->xmlChildrenNode,
                                          1);
                  sequence_item_data->tag = g_strdup ((gchar *) name_data);
                  xmlFree (name_data);
                  name_data = NULL;
                }

              if (xmlStrEqual
                  (name, (const xmlChar *) "use_external_velocity"))
                {
                  /* For the Start Sound sequence item, if this is set to 1
                   * we use the velocity of an external Note On message to
                   * scale the volume of the sound.  */
                  name_data =
                    xmlNodeListGetString (sequence_file,
                                          sequence_item_loc->xmlChildrenNode,
                                          1);
                  if (name_data != NULL)
                    {
                      long_data =
                        g_ascii_strtoll ((gchar *) name_data, NULL, 10);
                      xmlFree (name_data);
                      sequence_item_data->use_external_velocity = long_data;
                      name_data = NULL;
                    }
                }

              if (xmlStrEqual (name, (const xmlChar *) "volume"))
                {
                  /* For the Start Sound sequence item, scale the sound
                   * designer's volume by this amount.  */
                  name_data =
                    xmlNodeListGetString (sequence_file,
                                          sequence_item_loc->xmlChildrenNode,
                                          1);
                  if (name_data != NULL)
                    {
                      double_data =
                        g_ascii_strtod ((gchar *) name_data, NULL);
                      xmlFree (name_data);
                      sequence_item_data->volume = double_data;
                      name_data = NULL;
                    }
                }

              if (xmlStrEqual (name, (const xmlChar *) "pan"))
                {
                  /* For the Start Sound sequence item, adjust the sound
                   * designer's pan by this amount.  */
                  name_data =
                    xmlNodeListGetString (sequence_file,
                                          sequence_item_loc->xmlChildrenNode,
                                          1);
                  if (name_data != NULL)
                    {
                      double_data =
                        g_ascii_strtod ((gchar *) name_data, NULL);
                      xmlFree (name_data);
                      sequence_item_data->pan = double_data;
                      name_data = NULL;
                    }
                }

              if (xmlStrEqual (name, (const xmlChar *) "program_number"))
                {
                  /* For the Start Sound and Offer Sound sequence items, 
                   * the program number of
                   * the cluster in which we display the sound.  The program
                   * number of the clusters being shown is controlled by
                   * the sound effects operator.  Unless there are a large
                   * number of clusters being used, let this value default
                   * to zero.  */
                  name_data =
                    xmlNodeListGetString (sequence_file,
                                          sequence_item_loc->xmlChildrenNode,
                                          1);
                  if (name_data != NULL)
                    {
                      long_data =
                        g_ascii_strtoll ((gchar *) name_data, NULL, 10);
                      xmlFree (name_data);
                      sequence_item_data->program_number = long_data;
                      name_data = NULL;
                    }
                }

              if (xmlStrEqual (name, (const xmlChar *) "bank_number"))
                {
                  /* For the Start Sound and Offer Sound sequence items, 
                   * the bank number of the cluster in which we display 
                   * the sound.  The bank
                   * number of the clusters being shown is controlled by
                   * the sound effects operator.  Unless there are a large
                   * number of clusters being used, let this value default
                   * to zero.  */
                  name_data =
                    xmlNodeListGetString (sequence_file,
                                          sequence_item_loc->xmlChildrenNode,
                                          1);
                  if (name_data != NULL)
                    {
                      long_data =
                        g_ascii_strtoll ((gchar *) name_data, NULL, 10);
                      xmlFree (name_data);
                      sequence_item_data->bank_number = long_data;
                      name_data = NULL;
                    }
                }

              if (xmlStrEqual (name, (const xmlChar *) "cluster_number"))
                {
                  /* For the Start Sound and Offer Sound sequence items, 
                   * the cluster number in which we display the sound.  
                   * If none is specified,
                   * one will be chosen at run time.  Use this to place
                   * a sound in the same cluster as a previous, related,
                   * sound.  For example, you might devote a particular
                   * cluster to ringing a telephone even though it doesn't
                   * ring throughtout the show.  */
                  name_data =
                    xmlNodeListGetString (sequence_file,
                                          sequence_item_loc->xmlChildrenNode,
                                          1);
                  if (name_data != NULL)
                    {
                      long_data =
                        g_ascii_strtoll ((gchar *) name_data, NULL, 10);
                      xmlFree (name_data);
                      sequence_item_data->cluster_number = long_data;
                      sequence_item_data->cluster_number_specified = TRUE;
                      name_data = NULL;
                    }
                }

              if (xmlStrEqual (name, (const xmlChar *) "next_completion"))
                {
                  /* The next sequence item to execute, when and if this
                   * sound completes normally.  */
                  name_data =
                    xmlNodeListGetString (sequence_file,
                                          sequence_item_loc->xmlChildrenNode,
                                          1);
                  sequence_item_data->next_completion =
                    g_strdup ((gchar *) name_data);
                  xmlFree (name_data);
                  name_data = NULL;
                }

              if (xmlStrEqual (name, (const xmlChar *) "next_terminated"))
                {
                  /* The next sequence item to execute, when and if this
                   * sound terminates due to an external event, such as
                   * a MIDI Note Off or the sound effects operator pressing
                   * his Stop key.  */
                  name_data =
                    xmlNodeListGetString (sequence_file,
                                          sequence_item_loc->xmlChildrenNode,
                                          1);
                  sequence_item_data->next_terminated =
                    g_strdup ((gchar *) name_data);
                  xmlFree (name_data);
                  name_data = NULL;
                }

              if (xmlStrEqual (name, (const xmlChar *) "next_starts"))
                {
                  /* The next sequence item to execute when this sound has
                   * started.  This can be used to fork the sequencer.
                   */
                  name_data =
                    xmlNodeListGetString (sequence_file,
                                          sequence_item_loc->xmlChildrenNode,
                                          1);
                  sequence_item_data->next_starts =
                    g_strdup ((gchar *) name_data);
                  xmlFree (name_data);
                  name_data = NULL;
                }

              if (xmlStrEqual (name, (const xmlChar *) "importance"))
                {
                  /* The importance of this sound to the sound effects
                   * operator.  The most important sound being played
                   * is displayed on the console.  */
                  name_data =
                    xmlNodeListGetString (sequence_file,
                                          sequence_item_loc->xmlChildrenNode,
                                          1);
                  if (name_data != NULL)
                    {
                      long_data =
                        g_ascii_strtoll ((gchar *) name_data, NULL, 10);
                      xmlFree (name_data);
                      sequence_item_data->importance = long_data;
                      name_data = NULL;
                    }
                }

              if (xmlStrEqual (name, (const xmlChar *) "Q_number"))
                {
                  /* The Q number of this sound, for MIDI Show Control.  */
                  name_data =
                    xmlNodeListGetString (sequence_file,
                                          sequence_item_loc->xmlChildrenNode,
                                          1);
                  sequence_item_data->Q_number =
                    g_strdup ((gchar *) name_data);
                  xmlFree (name_data);
                  name_data = NULL;
                }

              if (xmlStrEqual (name, (const xmlChar *) "text_to_display"))
                {
                  /* The text to display to the sound effects operator when
                   * this sound is playing.  */
                  name_data =
                    xmlNodeListGetString (sequence_file,
                                          sequence_item_loc->xmlChildrenNode,
                                          1);
                  sequence_item_data->text_to_display =
                    g_strdup ((gchar *) name_data);
                  xmlFree (name_data);
                  name_data = NULL;
                }

              if (xmlStrEqual (name, (const xmlChar *) "next"))
                {
                  /* In other than the Start Sound sequence item, the next
                   * seqeunce item to execute when this one is done.  The
                   * Start Sound sequence item has three specialized next
                   * sequence items, and so does not use this general one.
                   */
                  name_data =
                    xmlNodeListGetString (sequence_file,
                                          sequence_item_loc->xmlChildrenNode,
                                          1);
                  sequence_item_data->next = g_strdup ((gchar *) name_data);
                  xmlFree (name_data);
                  name_data = NULL;
                }

              if (xmlStrEqual (name, (const xmlChar *) "time_to_wait"))
                {
                  /* In the Wait sequence item, the length of time to wait,
                   * in nanoseconds.  */
                  name_data =
                    xmlNodeListGetString (sequence_file,
                                          sequence_item_loc->xmlChildrenNode,
                                          1);
                  if (name_data != NULL)
                    {
                      long_data =
                        g_ascii_strtoll ((gchar *) name_data, NULL, 10);
                      xmlFree (name_data);
                      sequence_item_data->time_to_wait = long_data;
                      name_data = NULL;
                    }
                }

              if (xmlStrEqual (name, (const xmlChar *) "next_to_start"))
                {
                  /* In the Offer Sound sequence item, the sequence item
                   * that is to be executed when the sound effects operator
                   * presses the Start button on the specified cluster.
                   * The sequence item can also be started remotely.  
                   * This sequence item, like Start Sound, can be used
                   * to fork the sequencer.  */
                  name_data =
                    xmlNodeListGetString (sequence_file,
                                          sequence_item_loc->xmlChildrenNode,
                                          1);
                  sequence_item_data->next_to_start =
                    g_strdup ((gchar *) name_data);
                  xmlFree (name_data);
                  name_data = NULL;
                }

              if (xmlStrEqual (name, (const xmlChar *) "MIDI_program_number"))
                {
                  /* In the Offer Sound sequence item, the MIDI program number
                   * of the MIDI Note On message that will trigger the
                   * specified sequence item.  */
                  name_data =
                    xmlNodeListGetString (sequence_file,
                                          sequence_item_loc->xmlChildrenNode,
                                          1);
                  if (name_data != NULL)
                    {
                      long_data =
                        g_ascii_strtoll ((gchar *) name_data, NULL, 10);
                      xmlFree (name_data);
                      sequence_item_data->MIDI_program_number = long_data;
                      name_data = NULL;
                    }
                }

              if (xmlStrEqual (name, (const xmlChar *) "MIDI_note_number"))
                {
                  /* In the Offer Sound sequence item, the MIDI note number
                   * of the MIDI Note On message that will trigger the
                   * specified sequence item.  */
                  name_data =
                    xmlNodeListGetString (sequence_file,
                                          sequence_item_loc->xmlChildrenNode,
                                          1);
                  if (name_data != NULL)
                    {
                      long_data =
                        g_ascii_strtoll ((gchar *) name_data, NULL, 10);
                      xmlFree (name_data);
                      sequence_item_data->MIDI_note_number = long_data;
                      sequence_item_data->MIDI_note_number_specified = TRUE;
                      name_data = NULL;
                    }
                }

              if (xmlStrEqual (name, (const xmlChar *) "OSC_name"))
                {
                  /* In the Offer Sound sequence item, the Open Show Control
                   * (OSC) name used to trigger the specified sequence item
                   * remotely.  */
                  name_data =
                    xmlNodeListGetString (sequence_file,
                                          sequence_item_loc->xmlChildrenNode,
                                          1);
                  sequence_item_data->OSC_name =
                    g_strdup ((gchar *) name_data);
                  xmlFree (name_data);
                  name_data = NULL;
                }

              if (xmlStrEqual (name, (const xmlChar *) "macro_number"))
                {
                  /* In the Offer Sound sequence item, the macro number used
                   * by the Fire command of MIDI Show Control to trigger
                   * the specified sequence item remotely.  */
                  name_data =
                    xmlNodeListGetString (sequence_file,
                                          sequence_item_loc->xmlChildrenNode,
                                          1);
                  if (name_data != NULL)
                    {
                      long_data =
                        g_ascii_strtoll ((gchar *) name_data, NULL, 10);
                      xmlFree (name_data);
                      sequence_item_data->macro_number = long_data;
                      name_data = NULL;
                    }
                }

              if (xmlStrEqual (name, (const xmlChar *) "function_key"))
                {
                  /* In the Offer Sound and Operator Wait sequence items, 
                   * the function key used to trigger the specified sequence 
                   * item remotely.  */
                  name_data =
                    xmlNodeListGetString (sequence_file,
                                          sequence_item_loc->xmlChildrenNode,
                                          1);
                  sequence_item_data->function_key =
                    g_strdup ((gchar *) name_data);
                  xmlFree (name_data);
                  name_data = NULL;
                }

              /* Ignore fields we don't recognize, so we can read future
               * XML files. */

              sequence_item_loc = sequence_item_loc->next;
            }

          /* Append this sequence item to the sequence.  */
          sequence_append_item (sequence_item_data, app);
        }
      sequence_loc = sequence_loc->next;
    }

  return;
}

/* Dig through the sound_effects program section of an equipment file 
 * to find the sound and sequence information.  */
static void
parse_program_info (xmlDocPtr equipment_file, gchar * equipment_file_name,
                    xmlNodePtr program_loc, GApplication * app)
{
  xmlChar *key;
  const xmlChar *name;
  xmlChar *prop_name;
  gchar *file_name;
  gchar *file_dirname;
  gchar *absolute_file_name;
  gint64 port_number;
  xmlNodePtr sounds_loc, sequence_loc;
  xmlDocPtr sounds_file, sequence_file;
  const xmlChar *root_name;
  const xmlChar *sounds_name, *sequence_name;
  gboolean sounds_section_parsed, sequence_section_parsed;

  /* We start at the children of a "program" section which has the
   * name "sound_effects". */
  /* We are looking for sound and sequence sections.  */

  file_name = NULL;
  file_dirname = NULL;
  absolute_file_name = NULL;
  prop_name = NULL;

  while (program_loc != NULL)
    {
      name = program_loc->name;
      if (xmlStrEqual (name, (const xmlChar *) "port"))
        {
          /* This is the "port" section within "program".  */
          key =
            xmlNodeListGetString (equipment_file,
                                  program_loc->xmlChildrenNode, 1);
          port_number = g_ascii_strtoll ((gchar *) key, NULL, 10);
          /* Tell the network module the new port number. */
          network_set_port (port_number, app);
          xmlFree (key);
        }

      if (xmlStrEqual (name, (const xmlChar *) "sounds"))
        {
          /* This is the "sounds" section within "program".  
           * It will have a reference to a sounds XML file,
           * content or both.  First process the referenced file. */
          xmlFree (prop_name);
          prop_name = xmlGetProp (program_loc, (const xmlChar *) "href");
          if (prop_name != NULL)
            {
              /* We have a file reference.  */
              g_free (file_name);
              file_name = g_strdup ((gchar *) prop_name);
              xmlFree (prop_name);
              prop_name = NULL;
              /* If the file name does not have an absolute path,
               * prepend the path of the equipment or project file.
               * This allows equipment and project files to be 
               * copied along with the files they reference. */
              if (g_path_is_absolute (file_name))
                {
                  g_free (absolute_file_name);
                  absolute_file_name = g_strdup (file_name);
                }
              else
                {
                  g_free (file_dirname);
                  file_dirname = g_path_get_dirname (equipment_file_name);
                  absolute_file_name =
                    g_build_filename (file_dirname, file_name, NULL);
                  g_free (file_dirname);
                  file_dirname = NULL;
                }
              g_free (file_name);
              file_name = NULL;

              /* Read the specified file as an XML file. */
              xmlLineNumbersDefault (1);
              xmlThrDefIndentTreeOutput (1);
              xmlKeepBlanksDefault (0);
              xmlThrDefTreeIndentString ("    ");
              sounds_file = xmlParseFile (absolute_file_name);
              if (sounds_file == NULL)
                {
                  g_printerr ("Load of sound file %s failed.\n",
                              absolute_file_name);
                  g_free (absolute_file_name);
                  absolute_file_name = NULL;
                  return;
                }

              /* Make sure the sounds file is valid, then extract
               * data from it. */
              sounds_loc = xmlDocGetRootElement (sounds_file);
              if (sounds_loc == NULL)
                {
                  g_printerr ("Empty sound file: %s.\n", absolute_file_name);
                  g_free (absolute_file_name);
                  absolute_file_name = NULL;
                  xmlFree (sounds_file);
                  return;
                }
              root_name = sounds_loc->name;
              if (!xmlStrEqual (root_name, (const xmlChar *) "show_control"))
                {
                  g_printerr ("Not a show_control file: %s; is %s.\n",
                              absolute_file_name, root_name);
                  xmlFree (sounds_file);
                  g_free (absolute_file_name);
                  absolute_file_name = NULL;
                  return;
                }
              /* Within the top-level show_control structure
               * should be a sounds structure.  If there isn't,
               * this isn't a sound file and must be rejected.  */
              sounds_loc = sounds_loc->xmlChildrenNode;
              sounds_name = NULL;
              sounds_section_parsed = FALSE;
              while (sounds_loc != NULL)
                {
                  sounds_name = sounds_loc->name;
                  if (xmlStrEqual (sounds_name, (const xmlChar *) "sounds"))
                    {
                      parse_sounds_info (sounds_file, absolute_file_name,
                                         sounds_loc->xmlChildrenNode, app);
                      sounds_section_parsed = TRUE;
                    }
                  sounds_loc = sounds_loc->next;
                }
              if (!sounds_section_parsed)
                {
                  g_printerr ("Not a sounds file: %s; is %s.\n",
                              absolute_file_name, sounds_name);
                }
              xmlFree (sounds_file);
            }
          /* Now process the content of the sounds section. */
          parse_sounds_info (equipment_file, equipment_file_name,
                             program_loc->xmlChildrenNode, app);
          g_free (absolute_file_name);
          absolute_file_name = NULL;
        }

      if (xmlStrEqual (name, (const xmlChar *) "sound_sequence"))
        {
          /* This is the "sound_sequence" section within "program".  
           * It will have a reference to a sound sequence XML file,
           * content or both.  First process the referenced file. */
          xmlFree (prop_name);
          prop_name = xmlGetProp (program_loc, (const xmlChar *) "href");
          if (prop_name != NULL)
            {
              /* We have a file reference.  */
              g_free (file_name);
              file_name = g_strdup ((gchar *) prop_name);
              xmlFree (prop_name);
              prop_name = NULL;

              /* If the file name does not have an absolute path,
               * prepend the path of the equipment or project file.
               * This allows equipment and project files to be 
               * copied along with the files they reference. */
              if (g_path_is_absolute (file_name))
                {
                  g_free (absolute_file_name);
                  absolute_file_name = g_strdup (file_name);
                }
              else
                {
                  g_free (file_dirname);
                  file_dirname = g_path_get_dirname (equipment_file_name);
                  absolute_file_name =
                    g_build_filename (file_dirname, file_name, NULL);
                  g_free (file_dirname);
                  file_dirname = NULL;
                }
              g_free (file_name);
              file_name = NULL;

              /* Read the specified file as an XML file. */
              xmlLineNumbersDefault (1);
              xmlThrDefIndentTreeOutput (1);
              xmlKeepBlanksDefault (0);
              xmlThrDefTreeIndentString ("    ");
              sequence_file = xmlParseFile (absolute_file_name);
              if (sequence_file == NULL)
                {
                  g_printerr ("Load of sound sequence file %s failed.\n",
                              absolute_file_name);
                  g_free (absolute_file_name);
                  absolute_file_name = NULL;
                  return;
                }

              /* Make sure the sound sequence file is valid, then extract
               * data from it. */
              sequence_loc = xmlDocGetRootElement (sequence_file);
              if (sequence_loc == NULL)
                {
                  g_printerr ("Empty sound sequence file: %s.\n",
                              absolute_file_name);
                  g_free (absolute_file_name);
                  absolute_file_name = NULL;
                  xmlFree (sequence_file);
                  return;
                }
              root_name = sequence_loc->name;
              if (!xmlStrEqual (root_name, (const xmlChar *) "show_control"))
                {
                  g_printerr ("Not a show_control file: %s; is %s.\n",
                              absolute_file_name, root_name);
                  xmlFree (sequence_file);
                  g_free (absolute_file_name);
                  absolute_file_name = NULL;
                  return;
                }
              /* Within the top-level show_control structure
               * should be a sound sequence structure.  If there isn't,
               * this isn't a sound sequence file and must be rejected.  */
              sequence_loc = sequence_loc->xmlChildrenNode;
              sequence_name = NULL;
              sequence_section_parsed = FALSE;
              while (sequence_loc != NULL)
                {
                  sequence_name = sequence_loc->name;
                  if (xmlStrEqual
                      (sequence_name, (const xmlChar *) "sound_sequence"))
                    {
                      parse_sequence_info (sequence_file, absolute_file_name,
                                           sequence_loc->xmlChildrenNode,
                                           app);
                      sequence_section_parsed = TRUE;
                    }
                  sequence_loc = sequence_loc->next;
                }
              if (!sequence_section_parsed)
                {
                  g_printerr ("Not a sound sequence file: %s; is %s.\n",
                              absolute_file_name, sequence_name);
                }
              xmlFree (sequence_file);
            }
          /* Now process the content of the sound sequence section. */
          parse_sequence_info (equipment_file, equipment_file_name,
                               program_loc->xmlChildrenNode, app);
          g_free (absolute_file_name);
          absolute_file_name = NULL;
        }

      program_loc = program_loc->next;
    }
}

/* Dig through an equipment xml file, or the equipment section of a project
 * xml file, looking for the sound effect player's sounds and network port.  
 * When we find the network port, tell the network module about it. 
 * When we find sounds, parse the description.  */
static void
parse_equipment_info (xmlDocPtr equipment_file, gchar * equipment_file_name,
                      xmlNodePtr equipment_loc, GApplication * app)
{
  xmlChar *key;
  const xmlChar *name;
  xmlNodePtr program_loc;
  xmlChar *program_id;

  /* We start at the children of an "equipment" section. */
  /* We are looking for version and program sections. */

  while (equipment_loc != NULL)
    {
      name = equipment_loc->name;
      if (xmlStrEqual (name, (const xmlChar *) "version"))
        {
          /* This is the "version" section within "equipment". */
          key =
            xmlNodeListGetString (equipment_file,
                                  equipment_loc->xmlChildrenNode, 1);
          if ((!g_str_has_prefix ((gchar *) key, (gchar *) "1.")))
            {
              g_printerr ("Version number of equipment is %s, "
                          "should start with 1.\n", key);
              return;
            }
          xmlFree (key);
        }
      if (xmlStrEqual (name, (const xmlChar *) "program"))
        {
          /* This is a "program" section.  We only care about the sound
           * effects program. */
          program_id = xmlGetProp (equipment_loc, (const xmlChar *) "id");
          if (xmlStrEqual (program_id, (const xmlChar *) "sound_effects"))
            {
              /* This is the section of the XML file that contains information
               * about the sound effects program.  */
              program_loc = equipment_loc->xmlChildrenNode;
              parse_program_info (equipment_file, equipment_file_name,
                                  program_loc, app);
            }
          xmlFree (program_id);
        }
      equipment_loc = equipment_loc->next;
    }

  return;
}

/* Dig through the project xml file looking for the equipment references.  
 * Parse each one, since the information we are looking for might be scattered 
 * among them.  */
static void
parse_project_info (xmlDocPtr project_file, gchar * project_file_name,
                    xmlNodePtr current_loc, GApplication * app)
{
  xmlChar *key;
  const xmlChar *name;
  xmlChar *prop_name;
  gchar *file_name;
  gchar *file_dirname;
  gchar *absolute_file_name;
  xmlNodePtr equipment_loc;
  xmlDocPtr equipment_file;
  gboolean found_equipment_section;
  const xmlChar *root_name;
  const xmlChar *equipment_name;
  gboolean equipment_section_parsed;

  /* We start at the children of the "project" section.  
   * Important child sections for our purposes are "version" and "equipment".  
   */
  found_equipment_section = FALSE;
  file_name = NULL;
  file_dirname = NULL;
  absolute_file_name = NULL;
  prop_name = NULL;

  while (current_loc != NULL)
    {
      name = current_loc->name;
      if (xmlStrEqual (name, (const xmlChar *) "version"))
        {
          /* This is the "version" section within "project".  We can only
           * interpret version 1 of the project section, so reject all other
           * versions.  The value after the decimal point doesn't matter,
           * since 1.1, for example, will be a compatible extension of 1.0.  */
          key =
            xmlNodeListGetString (project_file, current_loc->xmlChildrenNode,
                                  1);
          if ((!g_str_has_prefix ((gchar *) key, (gchar *) "1.")))
            {
              g_printerr ("Version number of project is %s, "
                          "should start with 1.\n", key);
              return;
            }
          xmlFree (key);
        }
      if (xmlStrEqual (name, (const xmlChar *) "equipment"))
        {
          /* This is an "equipment" section within "project". 
             It will have a reference to an equipment XML file,
             content, or both.  First process the referenced file.  */
          found_equipment_section = TRUE;
          xmlFree (prop_name);
          prop_name = xmlGetProp (current_loc, (const xmlChar *) "href");
          if (prop_name != NULL)
            {
              g_free (file_name);
              file_name = g_strdup ((gchar *) prop_name);
              xmlFree (prop_name);
              prop_name = NULL;

              /* If the file name specified does not have an absolute path,
               * prepend the path to the project file.  This allows project 
               * files to be copied along with the files they reference.  */
              if (g_path_is_absolute (file_name))
                {
                  g_free (absolute_file_name);
                  absolute_file_name = g_strdup (file_name);
                }
              else
                {
                  g_free (file_dirname);
                  file_dirname = g_path_get_dirname (project_file_name);
                  absolute_file_name =
                    g_build_filename (file_dirname, file_name, NULL);
                  g_free (file_dirname);
                  file_dirname = NULL;
                }
              g_free (file_name);
              file_name = NULL;

              /* Read the specified file as an XML file. */
              xmlLineNumbersDefault (1);
              xmlThrDefIndentTreeOutput (1);
              xmlKeepBlanksDefault (0);
              xmlThrDefTreeIndentString ("    ");
              equipment_file = xmlParseFile (absolute_file_name);
              if (equipment_file == NULL)
                {
                  g_printerr ("Load of equipment file %s failed.\n",
                              absolute_file_name);
                  g_free (absolute_file_name);
                  absolute_file_name = NULL;
                  return;
                }

              /* Make sure the equipment file is valid, then extract data from 
               * it.  */
              equipment_loc = xmlDocGetRootElement (equipment_file);
              if (equipment_loc == NULL)
                {
                  g_printerr ("Empty equipment file: %s.\n",
                              absolute_file_name);
                  xmlFree (equipment_file);
                  g_free (absolute_file_name);
                  absolute_file_name = NULL;
                  return;
                }
              root_name = equipment_loc->name;
              if (!xmlStrEqual (root_name, (const xmlChar *) "show_control"))
                {
                  g_printerr ("Not a show_control file: %s; is %s.\n",
                              absolute_file_name, root_name);
                  xmlFree (equipment_file);
                  g_free (absolute_file_name);
                  absolute_file_name = NULL;
                  return;
                }

              /* Within the top-level show_control structure should be an 
               * equipment structure.  If there isn't, this isn't an equipment 
               * file and must be rejected. */
              equipment_loc = equipment_loc->xmlChildrenNode;
              equipment_name = NULL;
              equipment_section_parsed = FALSE;
              while (equipment_loc != NULL)
                {
                  equipment_name = equipment_loc->name;
                  if (xmlStrEqual
                      (equipment_name, (const xmlChar *) "equipment"))
                    {
                      parse_equipment_info (equipment_file,
                                            absolute_file_name,
                                            equipment_loc->xmlChildrenNode,
                                            app);
                      equipment_section_parsed = TRUE;
                    }
                  equipment_loc = equipment_loc->next;
                }
              if (!equipment_section_parsed)
                {
                  g_printerr ("Not an equipment file: %s; is %s.\n",
                              absolute_file_name, equipment_name);
                }
              xmlFree (equipment_file);
            }

          /* Now process the content of the equipment section. */
          parse_equipment_info (project_file, project_file_name,
                                current_loc->xmlChildrenNode, app);
          g_free (absolute_file_name);
          absolute_file_name = NULL;
        }
      current_loc = current_loc->next;
    }
  if (!found_equipment_section)
    {
      g_printerr ("No equipment section in project file: %s.\n",
                  absolute_file_name);
    }
  g_free (absolute_file_name);
  absolute_file_name = NULL;

  return;
}

/* Open a project file and read its contents.  The file is assumed to be in 
 * XML format. */
void
parse_xml_read_project_file (gchar * project_file_name, GApplication * app)
{
  xmlDocPtr project_file;
  xmlNodePtr current_loc;
  const xmlChar *name;
  gboolean project_section_parsed;

  /* Read the file as an XML file. */
  xmlLineNumbersDefault (1);
  xmlThrDefIndentTreeOutput (1);
  xmlKeepBlanksDefault (0);
  xmlThrDefTreeIndentString ("    ");
  project_file = xmlParseFile (project_file_name);
  if (project_file == NULL)
    {
      g_printerr ("Load of project file %s failed.\n", project_file_name);
      g_free (project_file_name);
      project_file_name = NULL;
      return;
    }

  /* Remember the file name. */
  sep_set_project_filename (project_file_name, app);

  /* Remember the data from the XML file, in case we want to refer to it
   * later. */
  sep_set_project_file (project_file, app);

  /* Make sure the project file is valid, then extract data from it. */
  current_loc = xmlDocGetRootElement (project_file);
  if (current_loc == NULL)
    {
      g_printerr ("Empty project file.\n");
      return;
    }
  name = current_loc->name;
  if (!xmlStrEqual (name, (const xmlChar *) "show_control"))
    {
      g_printerr ("Not a show_control file: %s.\n", current_loc->name);
      return;
    }

  /* Within the top-level show_control section should be a project
   * section.  If there isn't, this isn't a project file and must
   * be rejected.  If there is, process it.  */
  current_loc = current_loc->xmlChildrenNode;
  name = NULL;
  project_section_parsed = FALSE;
  while (current_loc != NULL)
    {
      name = current_loc->name;
      if (xmlStrEqual (name, (const xmlChar *) "project"))
        {
          parse_project_info (project_file, project_file_name,
                              current_loc->xmlChildrenNode, app);
          project_section_parsed = TRUE;
        }
      current_loc = current_loc->next;
    }
  if (!project_section_parsed)
    {
      g_printerr ("Not a project file: %s.\n", name);
    }

  return;
}

/* Write the project information to an XML file. */
void
parse_xml_write_project_file (gchar * project_file_name, GApplication * app)
{
  xmlDocPtr project_file;
  xmlNodePtr current_loc;
  xmlNodePtr equipment_loc;
  xmlNodePtr project_loc;
  xmlNodePtr program_loc;
  const xmlChar *name = NULL;
  xmlChar *prop_name = NULL;
  gint port_number;
  gchar *port_number_text = NULL;
  gboolean port_number_found;
  gchar text_buffer[G_ASCII_DTOSTR_BUF_SIZE];

  /* Write the project data as an XML file. */
  project_file = sep_get_project_file (app);
  if (project_file == NULL)
    {
      /* We don't have a project file--create one. */
      xmlLineNumbersDefault (1);
      xmlThrDefIndentTreeOutput (1);
      xmlKeepBlanksDefault (0);
      xmlThrDefTreeIndentString ("    ");
      project_file =
        xmlParseDoc ((xmlChar *) "<?xml version=\"1.0\" "
                     "encoding=\"utf-8\"?> <show_control> <project>"
                     "<version>1.0</version>"
                     "<equipment> <program id=\"sound_effects\">"
                     "<port>1500</port> </program> </equipment>"
                     "</project> </show_control>");
      sep_set_project_file (project_file, app);
    }

  /* The network port might have been changed using the preferences
   * dialogue.  Make sure we write out the current value. */

  /* Find the network node, then set the value of the port node 
   * within it.  This only works if the node number is in the top-level
   * project file.  */
  port_number_found = FALSE;
  current_loc = xmlDocGetRootElement (project_file);
  /* We know the root element is show_control, and there is only one,
   * so we don't have to check it or iterate over it.  */
  current_loc = current_loc->xmlChildrenNode;
  while (current_loc != NULL)
    {
      name = current_loc->name;
      if (xmlStrEqual (name, (const xmlChar *) "project"))
        {
          project_loc = current_loc->xmlChildrenNode;
          while (project_loc != NULL)
            {
              name = project_loc->name;
              if (xmlStrEqual (name, (const xmlChar *) "equipment"))
                {
                  equipment_loc = project_loc->xmlChildrenNode;
                  while (equipment_loc != NULL)
                    {
                      name = equipment_loc->name;
                      if (xmlStrEqual (name, (const xmlChar *) "program"))
                        {
                          prop_name =
                            xmlGetProp (equipment_loc,
                                        (const xmlChar *) "id");
                          if (xmlStrEqual
                              (prop_name, (const xmlChar *) "sound_effects"))
                            {
                              program_loc = equipment_loc->xmlChildrenNode;
                              while (program_loc != NULL)
                                {
                                  name = program_loc->name;
                                  if (xmlStrEqual
                                      (name, (const xmlChar *) "port"))
                                    {
                                      /* This is the "port" node within 
                                       * program sound_effects.  */
                                      port_number = network_get_port (app);
                                      port_number_text =
                                        g_ascii_dtostr (text_buffer,
                                                        G_ASCII_DTOSTR_BUF_SIZE,
                                                        (1.0 * port_number));
                                      xmlNodeSetContent (program_loc,
                                                         (xmlChar *)
                                                         port_number_text);
                                      port_number_found = TRUE;
                                    }
                                  program_loc = program_loc->next;
                                }
                            }
                        }
                      equipment_loc = equipment_loc->next;
                    }
                }
              project_loc = project_loc->next;
            }
        }
      current_loc = current_loc->next;
    }

  if (port_number_found)
    {
      /* Write the file, with indentations to make it easier to edit
       * manually. */
      xmlSaveFormatFileEnc (project_file_name, project_file, "utf-8", 1);

      /* Remember the file name so we can use it as the default
       * next time. */
      sep_set_project_filename (project_file_name, app);
    }
  else
    {
      g_printerr ("The project file is complex, and must be edited "
                  "with an XML editor such as Emacs.\n");
    }
  g_free (prop_name);

  return;
}
