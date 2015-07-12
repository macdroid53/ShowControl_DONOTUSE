/*
 * sequence_subroutines.c
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
#include <gst/gst.h>
#include "sequence_subroutines.h"
#include "sequence_structure.h"
#include "sound_effects_player.h"
#include "display_subroutines.h"
#include "sound_structure.h"
#include "sound_subroutines.h"
#include "button_subroutines.h"

/* When debugging it can be useful to trace what is happening in the
 * internal sequencer.  */
#define TRACE_SEQUENCER FALSE

/* the persistent data used by the internal sequencer */
struct sequence_info
{
  GList *item_list;             /* The sequence  */
  gchar *next_item_name;        /* The name of the next sequence item
                                 * to be executed.  */
  GList *offering;              /* The list of Offer Sound items 
                                 * that are still attached to a cluster  */
  GList *running;               /* The list of Start Sound items
                                 * that are still attached to a cluster  */
  struct remember_info *current_operator_wait;  /* The Operator Wait sequence 
                                                 * item that is currently 
                                                 * displaying its text to the 
                                                 * operator.  */
  GList *operator_waiting;      /* The Operator Wait sequence items that are
                                 * waiting for their turn at the operator.  */
};

/* an entry on the running, offering or operator waiting lists */
struct remember_info
{
  guint cluster_number;
  struct sound_info *sound_effect;
  struct sequence_item_info *sequence_item;
};

/* Forward declarations, so I can call these subroutines before I define them.  
 */

static struct sequence_item_info *find_item_by_name (gchar * item_name,
                                                     struct sequence_info
                                                     *sequence_data);

static void execute_items (struct sequence_info *sequence_data,
                           GApplication * app);

static void execute_item (struct sequence_item_info *the_item,
                          struct sequence_info *sequence_data,
                          GApplication * app);

static void execute_start_sound (struct sequence_item_info *the_item,
                                 struct sequence_info *sequence_data,
                                 GApplication * app);

static void execute_stop_sound (struct sequence_item_info *the_item,
                                struct sequence_info *sequence_data,
                                GApplication * app);

static void execute_offer_sound (struct sequence_item_info *the_item,
                                 struct sequence_info *sequence_data,
                                 GApplication * app);

static void execute_cease_offering_sound (struct sequence_item_info *the_item,
                                          struct sequence_info *sequence_data,
                                          GApplication * app);

static void execute_operator_wait (struct sequence_item_info *the_item,
                                   struct sequence_info *sequence_data,
                                   GApplication * app);

/* Subroutines for handling sequence items.  */

/* Initialize the internal sequencer.  */
void *
sequence_init (GApplication * app)
{
  struct sequence_info *sequence_data;

  sequence_data = g_malloc (sizeof (struct sequence_info));
  sequence_data->item_list = NULL;
  sequence_data->offering = NULL;
  sequence_data->running = NULL;
  sequence_data->current_operator_wait = NULL;
  sequence_data->operator_waiting = NULL;
  return (sequence_data);
}

/* Append a sequence item to the sequence. */
void
sequence_append_item (struct sequence_item_info *item, GApplication * app)
{
  struct sequence_info *sequence_data;

  sequence_data = sep_get_sequence_data (app);
  sequence_data->item_list = g_list_append (sequence_data->item_list, item);
  return;
}

/* Start running the sequencer.  */
void
sequence_start (GApplication * app)
{
  struct sequence_info *sequence_data;
  GList *item_list;
  struct sequence_item_info *item, *start_item;

  sequence_data = sep_get_sequence_data (app);

  /* Find the sequence start item in the sequence.  */
  start_item = NULL;
  for (item_list = sequence_data->item_list; item_list != NULL;
       item_list = item_list->next)
    {
      item = item_list->data;
      if (item->type == start_sequence)
        {
          start_item = item;
          break;
        }
    }
  if (start_item == NULL)
    {
      display_show_message ("No sequence start item.", app);
      return;
    }

  /* We have a sequence which contains a start sequence item.  Proceed to
   * the specified next item.  */
  if (start_item->next == NULL)
    {
      display_show_message ("Sequence start has no next item.", app);
      return;
    }

  sequence_data->next_item_name = start_item->next;

  /* Run sequence items starting at next_item_name.  */
  execute_items (sequence_data, app);

  return;
}

/* Execute the named next item, and continue execution until we must
 * wait for something.  */
static void
execute_items (struct sequence_info *sequence_data, GApplication * app)
{
  struct sequence_item_info *next_item;

  while (sequence_data->next_item_name != NULL)
    {
      next_item =
        find_item_by_name (sequence_data->next_item_name, sequence_data);

      if (next_item == NULL)
        {
          display_show_message ("Next item not found.", app);
          break;
        }

      execute_item (next_item, sequence_data, app);
    }

  return;
}

/* Find a sequence item, given its name.  If the item is not found,
 * returns NULL.  */
struct sequence_item_info *
find_item_by_name (gchar * item_name, struct sequence_info *sequence_data)
{

  struct sequence_item_info *item, *found_item;
  GList *item_list;

  if (TRACE_SEQUENCER)
    {
      g_print ("Searching for item %s.\n", item_name);
    }

  found_item = NULL;
  for (item_list = sequence_data->item_list; item_list != NULL;
       item_list = item_list->next)
    {
      item = item_list->data;

      if (g_strcmp0 (item_name, item->name) == 0)
        {
          found_item = item;
          break;
        }
    }

  return (found_item);
}

/* Execute a sequence item.  */
void
execute_item (struct sequence_item_info *the_item,
              struct sequence_info *sequence_data, GApplication * app)
{

  switch (the_item->type)
    {
    case unknown:
      display_show_message ("Unknown sequence item", app);
      sequence_data->next_item_name = NULL;
      break;

    case start_sound:
      execute_start_sound (the_item, sequence_data, app);
      break;

    case stop:
      execute_stop_sound (the_item, sequence_data, app);
      break;

    case wait:
      display_show_message ("Wait", app);
      sequence_data->next_item_name = NULL;
      break;

    case offer_sound:
      execute_offer_sound (the_item, sequence_data, app);
      break;

    case cease_offering_sound:
      execute_cease_offering_sound (the_item, sequence_data, app);
      break;

    case operator_wait:
      execute_operator_wait (the_item, sequence_data, app);
      break;

    case start_sequence:
      display_show_message ("Start sequence", app);
      sequence_data->next_item_name = NULL;
      break;

    }

  return;
}

/* Execute a start sound sequence item.  */
void
execute_start_sound (struct sequence_item_info *the_item,
                     struct sequence_info *sequence_data, GApplication * app)
{
  gint cluster_number;
  struct sound_info *sound_effect;
  struct remember_info *remember_data;

  if (TRACE_SEQUENCER)
    {
      g_print ("start sound, cluster = %d, sound name = %s, next = %s, "
               " complete = %s, terminate = %s.\n", the_item->cluster_number,
               the_item->sound_name, the_item->next_starts,
               the_item->next_completion, the_item->next_termination);
    }

  cluster_number = the_item->cluster_number;

  /* Set the name of the cluster to the specified text.  */
  sound_cluster_set_name (the_item->text_to_display, cluster_number, app);

  /* Associate the sound with the cluster.  */
  sound_effect =
    sound_bind_to_cluster (the_item->sound_name, cluster_number, app);

  /* Start that sound.  */
  sound_start_playing (sound_effect, app);

  /* Show the operator that a sound is playing on this cluster.  */
  button_set_cluster_playing (sound_effect, app);

  /* Remember that the sound is running.  */
  remember_data = g_malloc (sizeof (struct remember_info));
  remember_data->cluster_number = cluster_number;
  remember_data->sequence_item = the_item;
  remember_data->sound_effect = sound_effect;
  sequence_data->running =
    g_list_append (sequence_data->running, remember_data);

  /* Advance to the next sequence item.  */
  sequence_data->next_item_name = the_item->next_starts;

  return;
}

/* Execute a Stop sequence item.  */
void
execute_stop_sound (struct sequence_item_info *the_item,
                    struct sequence_info *sequence_data, GApplication * app)
{

  struct remember_info *remember_data;
  struct sequence_item_info *sequence_item;
  gboolean item_found;
  GList *item_list;

  /* Find the running sound whose Start Sound sequence item has the specified
   * tag.  */
  item_found = FALSE;
  for (item_list = sequence_data->running; item_list != NULL;
       item_list = item_list->next)
    {
      remember_data = item_list->data;
      sequence_item = remember_data->sequence_item;
      if (g_strcmp0 (the_item->tag, sequence_item->tag) == 0)
        {
          item_found = TRUE;
          break;
        }
    }
  if (!item_found)
    {
      display_show_message ("No matching tag.", app);
      return;
    }

  /* Stop the sound.  When the sound terminates we will clean up.  */
  sound_stop_playing (remember_data->sound_effect, app);

  /* Advance to the next sequence item.  */
  sequence_data->next_item_name = the_item->next;

  return;
}

/* Execute an Offer Sound sequence item.  */
void
execute_offer_sound (struct sequence_item_info *the_item,
                     struct sequence_info *sequence_data, GApplication * app)
{
  gint cluster_number;
  struct remember_info *remember_data;

  if (TRACE_SEQUENCER)
    {
      g_print ("Offer sound, name = %s, cluster = %d, Q number = %s, "
               "next = %s.\n", the_item->name, the_item->cluster_number,
               the_item->Q_number, the_item->next_to_start);
    }

  cluster_number = the_item->cluster_number;

  /* Set the name of the cluster to the specified text.  */
  sound_cluster_set_name (the_item->text_to_display, cluster_number, app);

  /* Remember that the sound is being offered.  */
  remember_data = g_malloc (sizeof (struct remember_info));
  remember_data->cluster_number = cluster_number;
  remember_data->sound_effect = NULL;
  remember_data->sequence_item = the_item;
  sequence_data->offering =
    g_list_append (sequence_data->offering, remember_data);

  /* Advance to the next sequence item.  */
  sequence_data->next_item_name = the_item->next;

  return;
}

/* Execute a Cease Offering Sound sequence item.  */
void
execute_cease_offering_sound (struct sequence_item_info *the_item,
                              struct sequence_info *sequence_data,
                              GApplication * app)
{
  gint cluster_number;
  struct remember_info *remember_data;
  struct sequence_item_info *sequence_item;
  GList *list_element, *next_list_element;

  if (TRACE_SEQUENCER)
    {
      g_print ("Cease offering sound, name = %s, tag = %s, " "next = %s.\n",
               the_item->name, the_item->tag, the_item->next);
    }

  /* Process every Offer Sound sequence item with the same tag.  */
  list_element = sequence_data->offering;
  while (list_element != NULL)
    {
      next_list_element = list_element->next;
      remember_data = list_element->data;
      sequence_item = remember_data->sequence_item;
      if (g_strcmp0 (the_item->tag, sequence_item->tag) == 0)
        {
          /* We have a match.  Remove the Offer Sound from the cluster.  */
          sequence_data->offering =
            g_list_remove_link (sequence_data->offering, list_element);

          /* Remove the Offer Sound's text from the cluster.  */
          cluster_number = remember_data->cluster_number;
          sound_cluster_set_name ((gchar *) "", cluster_number, app);

          g_list_free (list_element);
          g_free (remember_data);
        }
      list_element = next_list_element;
    }

  /* Advance to the next sequence item.  */
  sequence_data->next_item_name = the_item->next;

  return;
}

/* Execute an Operator Wait sequence item.  */
void
execute_operator_wait (struct sequence_item_info *the_item,
                       struct sequence_info *sequence_data,
                       GApplication * app)
{
  struct remember_info *remember_data;

  if (TRACE_SEQUENCER)
    {
      g_print ("Operator Wait, name = %s, next play = %s, "
               "operator text = %s, next = %s.\n", the_item->name,
               the_item->next_play, the_item->text_to_display,
               the_item->next);
    }

  /* Record information about the wait, since we will need it when
   * the wait is over.  */
  remember_data = g_malloc (sizeof (struct remember_info));
  remember_data->cluster_number = 0;
  remember_data->sound_effect = NULL;
  remember_data->sequence_item = the_item;

  if (sequence_data->current_operator_wait == NULL)
    {
      /* There are no prior operator wait commands running.  */
      sequence_data->current_operator_wait = remember_data;
      display_set_operator_text (the_item->text_to_display, app);
    }
  else
    {
      /* There is an Operator Wait already running; place this one
       * on the list to be executed later.  */
      sequence_data->operator_waiting =
        g_list_append (sequence_data->operator_waiting, remember_data);
    }

  /* Advance to the next sequence item.  */
  sequence_data->next_item_name = the_item->next;

  return;
}

/* Execute the Go command from an external sequencer issuing MIDI Show Control
 * commands.  */
void
sequence_MIDI_show_control_go (gchar * Q_number, GApplication * app)
{
  struct sequence_info *sequence_data;
  struct remember_info *remember_data;
  struct sequence_item_info *sequence_item;
  gboolean found_item;
  GList *item_list;

  sequence_data = sep_get_sequence_data (app);

  if (TRACE_SEQUENCER)
    {
      g_print ("MIDI show control go, Q_number = %s.\n", Q_number);
    }

  /* Find the cluster whose Offer Sound sequence item has the specified
   * Q_number.  */
  found_item = FALSE;
  for (item_list = sequence_data->offering; item_list != NULL;
       item_list = item_list->next)
    {
      remember_data = item_list->data;
      sequence_item = remember_data->sequence_item;
      if (g_strcmp0 (Q_number, sequence_item->Q_number) == 0)
        {
          found_item = TRUE;
          break;
        }
    }

  if (!found_item)
    {
      display_show_message ("No matching Q_number.", app);
      return;
    }

  /* Run the sequencer.  A subsequent Start Sound sequence item
   * which names this same cluster will take posession of the cluster
   * until it completes or is terminated.  */
  sequence_data->next_item_name = sequence_item->next_to_start;
  execute_items (sequence_data, app);

  return;
}

/* Execute the Go_off command from an external sequencer issuing MIDI Show 
 * Control commands.  */
void
sequence_MIDI_show_control_go_off (gchar * Q_number, GApplication * app)
{
  struct sequence_info *sequence_data;
  struct remember_info *remember_data;
  struct sequence_item_info *sequence_item;
  GList *item_list;

  sequence_data = sep_get_sequence_data (app);

  /* Stop the running sounds whose Start Sound sequence item has the specified
   * Q_number.  If there is no Q number, stop all sounds.  */
  for (item_list = sequence_data->running; item_list != NULL;
       item_list = item_list->next)
    {
      remember_data = item_list->data;
      sequence_item = remember_data->sequence_item;
      if ((Q_number == NULL) || (g_strcmp0 (Q_number, (gchar *) ""))
          || (g_strcmp0 (Q_number, sequence_item->Q_number) == 0))
        {
          /* Stop the sound.  When the sound terminates we will clean up.  */
          sound_stop_playing (remember_data->sound_effect, app);
        }
    }

  return;
}

/* Process the Start button on a cluster.  */
void
sequence_cluster_start (guint cluster_number, GApplication * app)
{
  struct sequence_info *sequence_data;
  struct remember_info *remember_data;
  struct sequence_item_info *sequence_item;
  gboolean found_item;
  GList *item_list;

  if (TRACE_SEQUENCER)
    {
      g_print ("sequence_cluster_start: cluster = %d.\n", cluster_number);
    }

  sequence_data = sep_get_sequence_data (app);

  /* See if there is an Offer Sound sequence item outstanding which names
   * this cluster.  */
  found_item = FALSE;
  for (item_list = sequence_data->offering; item_list != NULL;
       item_list = item_list->next)
    {
      remember_data = item_list->data;
      if (remember_data->cluster_number == cluster_number)
        {
          found_item = TRUE;
          break;
        }
    }
  if (!found_item)
    {
      display_show_message ("No sound offering on this cluster.", app);
      return;
    }

  /* We have an Offer Sound sequence item on this cluster.
   * Run the sequencer starting at its specified sequence item.  */
  sequence_item = remember_data->sequence_item;
  sequence_data->next_item_name = sequence_item->next_to_start;
  execute_items (sequence_data, app);

  return;
}

/* Process the Stop button on a cluster.  */
void
sequence_cluster_stop (guint cluster_number, GApplication * app)
{
  struct sequence_info *sequence_data;
  struct remember_info *remember_data;
  gboolean item_found;
  GList *item_list;

  sequence_data = sep_get_sequence_data (app);

  /* See if there is a Start Sound sequence item outstanding which names
   * this cluster.  */
  item_found = FALSE;
  for (item_list = sequence_data->running; item_list != NULL;
       item_list = item_list->next)
    {
      remember_data = item_list->data;
      if (remember_data->cluster_number == cluster_number)
        {
          item_found = TRUE;
          break;
        }
    }
  if (!item_found)
    {
      /* There isn't.  Ignore the stop button.  */
      display_show_message ("No sound to stop.", app);
      return;
    }

  /* We have a Start Sound sequence item on this cluster.
   * Tell the sound to stop.  When it has stopped, the termination
   * process will be invoked.  */
  sound_stop_playing (remember_data->sound_effect, app);

  return;
}

/* Process the Play button.  */
void
sequence_button_play (GApplication * app)
{
  struct sequence_info *sequence_data;
  struct remember_info *remember_data;
  struct sequence_item_info *current_sequence_item;
  struct sequence_item_info *next_sequence_item;

  sequence_data = sep_get_sequence_data (app);
  remember_data = sequence_data->current_operator_wait;
  GList *list_element;

  /* If we are not waiting for the operator to press the key,
   * do nothing.  */
  if (remember_data == NULL)
    return;

  current_sequence_item = remember_data->sequence_item;

  /* This Operator Wait sequence item is no longer waiting.  */
  sequence_data->current_operator_wait = NULL;
  g_free (remember_data);
  remember_data = NULL;

  /* See if there is another one ready to wait.  */
  list_element = g_list_first (sequence_data->operator_waiting);
  if (list_element != NULL)
    {
      /* There is, give it its chance to display for the opeaator.  */
      sequence_data->operator_waiting =
        g_list_remove_link (sequence_data->operator_waiting, list_element);
      remember_data = list_element->data;
      next_sequence_item = remember_data->sequence_item;
      display_set_operator_text (next_sequence_item->text_to_display, app);
      sequence_data->current_operator_wait = remember_data;
      g_list_free (list_element);
    }
  else
    {
      display_clear_operator_text (app);
    }

  /* Run the sequencer starting from the Operator Wait's specified label.  */
  sequence_data->next_item_name = current_sequence_item->next_play;
  execute_items (sequence_data, app);

  return;
}

/* Process the completion of a sound.  */
void
sequence_sound_completion (struct sound_info *sound_effect,
                           GApplication * app)
{
  struct sequence_info *sequence_data;
  struct remember_info *remember_data;
  struct sequence_item_info *start_sound_sequence_item;
  struct sequence_item_info *offer_sound_sequence_item;
  gboolean item_found;
  GList *item_list, *found_item;

  sequence_data = sep_get_sequence_data (app);

  if (TRACE_SEQUENCER)
    {
      g_print ("completion of sound %s.\n", sound_effect->name);
    }

  /* See if there is a Start Sound sequence item outstanding which names
   * this sound.  */
  item_found = FALSE;
  for (item_list = sequence_data->running; item_list != NULL;
       item_list = item_list->next)
    {
      remember_data = item_list->data;
      if (remember_data->sound_effect == sound_effect)
        {
          found_item = item_list;
          item_found = TRUE;
          break;
        }
    }

  if (!item_found)
    {
      /* There isn't.  Ignore the completion.  */
      display_show_message ("Completion but sound not running.", app);
      return;
    }

  /* We have a Start Sound sequence item for this sound.  It has
   * completed.  */
  start_sound_sequence_item = remember_data->sequence_item;

  /* Remove the sequence item from the running list.  */
  sequence_data->running =
    g_list_remove_link (sequence_data->running, found_item);
  g_free (remember_data);
  g_list_free (found_item);

  /* Set the start label on the cluster back to "Start".  */
  button_reset_cluster (sound_effect, app);

  /* See if there is an Offer Sound sequence item outstanding which names
   * this cluster.  */
  item_found = FALSE;
  for (item_list = sequence_data->offering; item_list != NULL;
       item_list = item_list->next)
    {
      remember_data = item_list->data;
      if (remember_data->cluster_number == sound_effect->cluster_number)
        {
          offer_sound_sequence_item = remember_data->sequence_item;
          item_found = TRUE;
          break;
        }
    }

  /* If there is, restore its text to the cluster.  If there isn't, clear
   * the text field.  */
  if (item_found)
    {
      sound_cluster_set_name (offer_sound_sequence_item->text_to_display,
                              remember_data->cluster_number, app);
    }
  else
    {
      sound_cluster_set_name ((gchar *) "", sound_effect->cluster_number,
                              app);
    }

  /* Now that the Start Sound has completed, run the sequencer
   * from its completion label.  */
  sequence_data->next_item_name = start_sound_sequence_item->next_completion;
  execute_items (sequence_data, app);

  return;
}

/* Process the termination of a sound.  */
void
sequence_sound_termination (struct sound_info *sound_effect,
                            GApplication * app)
{
  struct sequence_info *sequence_data;
  struct remember_info *remember_data;
  struct sequence_item_info *start_sound_sequence_item;
  struct sequence_item_info *offer_sound_sequence_item;
  gboolean item_found;
  GList *item_list, *found_item;

  sequence_data = sep_get_sequence_data (app);

  if (TRACE_SEQUENCER)
    {
      g_print ("termination of sound %s.\n", sound_effect->name);
    }

  /* See if there is a Start Sound sequence item outstanding which names
   * this cluster.  */
  item_found = FALSE;
  for (item_list = sequence_data->running; item_list != NULL;
       item_list = item_list->next)
    {
      remember_data = item_list->data;
      if (remember_data->sound_effect == sound_effect)
        {
          found_item = item_list;
          item_found = TRUE;
          break;
        }
    }

  if (!item_found)
    {
      /* There isn't.  Ignore the termination.  */
      display_show_message ("Termination but sound not running.", app);
      return;
    }

  /* We have a Start Sound sequence item for this sound.  It has
   * terminated.  */
  start_sound_sequence_item = remember_data->sequence_item;

  /* Remove the sequence item from the running list.  */
  sequence_data->running =
    g_list_remove_link (sequence_data->running, found_item);
  g_free (remember_data);
  g_list_free (found_item);

  /* Set the start label on the cluster back to "Start".  */
  button_reset_cluster (sound_effect, app);

  /* See if there is an Offer Sound sequence item outstanding which names
   * this cluster.  */
  item_found = FALSE;
  for (item_list = sequence_data->offering; item_list != NULL;
       item_list = item_list->next)
    {
      remember_data = item_list->data;
      if (remember_data->cluster_number == sound_effect->cluster_number)
        {
          offer_sound_sequence_item = remember_data->sequence_item;
          item_found = TRUE;
          break;
        }
    }

  /* If there is, restore its text to the cluster.  If there isn't, erase
   * the text field.  */
  if (item_found)
    {
      sound_cluster_set_name (offer_sound_sequence_item->text_to_display,
                              remember_data->cluster_number, app);
    }
  else
    {
      sound_cluster_set_name ((gchar *) "", sound_effect->cluster_number,
                              app);
    }

  /* Now that the Start Sound has terminated, run the sequencer
   * from its termination label.  */
  sequence_data->next_item_name = start_sound_sequence_item->next_termination;
  execute_items (sequence_data, app);

  return;
}
