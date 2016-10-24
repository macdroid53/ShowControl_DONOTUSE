/*
 * sound_subroutines.c
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

#include <stdlib.h>
#include <gst/gst.h>
#include "sound_subroutines.h"
#include "sound_structure.h"
#include "sound_effects_player.h"
#include "gstreamer_subroutines.h"
#include "button_subroutines.h"
#include "display_subroutines.h"
#include "sequence_subroutines.h"

/* Subroutines for processing sounds.  */

/* Initialize the sound system.  We have already read an XML file
 * containing sound definitions and put the results in the sound list.  */
GstPipeline *
sound_init (GApplication * app)
{
  GstPipeline *pipeline_element;
  GstBin *bin_element;
  GList *sound_list;
  gint sound_number, sound_count;
  GList *l;
  struct sound_info *sound_data;

  sound_list = sep_get_sound_list (app);

  /* Count the non-disabled sounds.  */
  sound_count = 0;
  for (l = sound_list; l != NULL; l = l->next)
    {
      sound_data = l->data;
      if (!sound_data->disabled)
        sound_count = sound_count + 1;
    }

  /* If we have any sounds, create the gstreamer pipeline and place the 
   * sound effects bins in it.  */
  if (sound_count == 0)
    {
      return NULL;
    }

  pipeline_element = gstreamer_init (sound_count, app);
  if (pipeline_element == NULL)
    {
      /* We are unable to create the gstreamer pipeline.  */
      return pipeline_element;
    }

  /* Create a gstreamer bin for each enabled sound effect and place it in
   * the gstreamer pipeline.  */
  sound_number = 0;
  for (l = sound_list; l != NULL; l = l->next)
    {
      sound_data = l->data;
      if (!sound_data->disabled)
        {
          bin_element =
            gstreamer_create_bin (sound_data, sound_number, pipeline_element,
                                  app);

          if (bin_element == NULL)
            {
              /* We are unable to create the gstreamer bin.  This might
               * be because an element is unavailable.  */
              sound_data->disabled = TRUE;
              sound_count = sound_count - 1;
              continue;
            }

          sound_data->sound_control = bin_element;
          sound_number = sound_number + 1;
        }
    }

  /* If we have any sound effects, complete the gstreamer pipeline.  */
  if (sound_count > 0)
    {
      gstreamer_complete_pipeline (pipeline_element, app);
    }
  else
    {
      /* Since we have no sound effects, we don't need a pipeline.  */
      g_object_unref (pipeline_element);
      pipeline_element = NULL;
    }

  return pipeline_element;
}

/* Set the name displayed in a cluster.  */
void
sound_cluster_set_name (gchar * sound_name, guint cluster_number,
                        GApplication * app)
{
  GList *children_list;
  const char *child_name;
  GtkLabel *title_label;
  GtkWidget *cluster;

  /* find the cluster */
  cluster = sep_get_cluster_from_number (cluster_number, app);

  if (cluster == NULL)
    return;

  /* Set the name in the cluster.  */
  title_label = NULL;
  children_list = gtk_container_get_children (GTK_CONTAINER (cluster));
  while (children_list != NULL)
    {
      child_name = gtk_widget_get_name (children_list->data);
      if (g_strcmp0 (child_name, "title") == 0)
        {
          title_label = children_list->data;
          break;
        }
      children_list = children_list->next;
    }
  g_list_free (children_list);

  if (title_label != NULL)
    {
      gtk_label_set_label (title_label, sound_name);
    }
}

/* Append a sound to the list of sounds. */
void
sound_append_sound (struct sound_info *sound_effect, GApplication * app)
{
  GList *sound_list;

  sound_list = sep_get_sound_list (app);
  sound_list = g_list_append (sound_list, sound_effect);
  sep_set_sound_list (sound_list, app);
  return;
}

/* Associate a sound with a specified cluster.  */
struct sound_info *
sound_bind_to_cluster (gchar * sound_name, guint cluster_number,
                       GApplication * app)
{
  GList *sound_effect_list;
  GtkWidget *cluster_widget;
  struct sound_info *sound_effect;
  gboolean sound_effect_found;

  sound_effect_list = sep_get_sound_list (app);
  sound_effect_found = FALSE;
  while (sound_effect_list != NULL)
    {
      sound_effect = sound_effect_list->data;
      if ((g_strcmp0 (sound_name, sound_effect->name)) == 0)
        {
          sound_effect_found = TRUE;
          break;
        }
      sound_effect_list = sound_effect_list->next;
    }

  if (!sound_effect_found)
    return NULL;

  cluster_widget = sep_get_cluster_from_number (cluster_number, app);
  sound_effect->cluster_number = cluster_number;
  sound_effect->cluster_widget = cluster_widget;

  return sound_effect;

}

/* Disassociate a sound from its cluster.  */
void
sound_unbind_from_cluster (struct sound_info *sound_effect,
                           GApplication * app)
{
  sound_effect->cluster_number = 0;
  sound_effect->cluster_widget = NULL;

  return;
}

/* Start playing a sound effect.  */
void
sound_start_playing (struct sound_info *sound_data, GApplication * app)
{
  GstBin *bin_element;
  GstEvent *event;
  GstStructure *structure;

  bin_element = sound_data->sound_control;
  if (bin_element == NULL)
    return;

  /* If the sound has already been started, and is not yet releasing, 
   * don't try to start it again.  A sound is releasing if we have sent
   * a release message or if it has entered its release stage on its own.  */
  if (sound_data->running && !sound_data->release_sent
      && !sound_data->release_has_started)
    {
      return;
    }

  /* Send a start message to the bin.  It will be routed to the source, and
   * flow from there downstream through the looper and envelope.  
   * The looper element will start sending its local buffer
   * and the envelope element will start to shape the volume.  */
  sound_data->running = TRUE;
  sound_data->release_sent = FALSE;
  sound_data->release_has_started = FALSE;
  structure = gst_structure_new_empty ((gchar *) "start");
  event = gst_event_new_custom (GST_EVENT_CUSTOM_UPSTREAM, structure);
  gst_element_send_event (GST_ELEMENT (bin_element), event);

  return;
}

/* Stop playing a sound effect.  */
void
sound_stop_playing (struct sound_info *sound_data, GApplication * app)
{
  GstBin *bin_element;
  GstEvent *event;
  GstStructure *structure;

  bin_element = sound_data->sound_control;

  /* Send a release message to the bin.  The looper element will stop
   * looping, and the envelope element will start shutting down the sound.
   * If the sound has a non-zero release time we should get a call to 
   * release_started shortly, unless the sound has already completed
   * and the message is still on its way down the pipeline.  */
  structure = gst_structure_new_empty ((gchar *) "release");
  event = gst_event_new_custom (GST_EVENT_CUSTOM_UPSTREAM, structure);
  gst_element_send_event (GST_ELEMENT (bin_element), event);

  sound_data->release_sent = TRUE;

  return;
}

/* Get the elapsed time of a playing sound.  */
gchar *
sound_get_elapsed_time (struct sound_info * sound_data, GApplication * app)
{
  GstElement *looper_element;
  gchar *string_value;

  looper_element = gstreamer_get_looper (sound_data->sound_control);
  g_object_get (looper_element, (gchar *) "elapsed-time", &string_value,
                NULL);
  return string_value;
}

/* Get the remaining run time of a playing sound.  */
gchar *
sound_get_remaining_time (struct sound_info * sound_data, GApplication * app)
{
  GstElement *looper_element;
  gchar *string_value;

  looper_element = gstreamer_get_looper (sound_data->sound_control);
  g_object_get (looper_element, (gchar *) "remaining-time", &string_value,
		NULL);
  return string_value;
}

/* Receive a completed message, which indicates that a sound has finished.  */
void
sound_completed (const gchar * sound_name, GApplication * app)
{
  GList *sound_effect_list;
  struct sound_info *sound_effect = NULL;
  gboolean sound_effect_found;

  /* Search through the sound effects for the one with this name.  */
  sound_effect_list = sep_get_sound_list (app);
  sound_effect_found = FALSE;
  while (sound_effect_list != NULL)
    {
      sound_effect = sound_effect_list->data;
      if (g_strcmp0 (sound_effect->name, sound_name) == 0)
        {
          sound_effect_found = TRUE;
          break;
        }
      sound_effect_list = sound_effect_list->next;
    }

  /* There isn't one--ignore the completion.  */
  if (!sound_effect_found)
    return;

  /* Flag that the sound is no longer playing.  */
  sound_effect->running = FALSE;

  /* Let the internal sequencer distinguish a sound that has completed
   * normally from one that has been stopped.  */
  sequence_sound_completion (sound_effect, sound_effect->release_sent, app);
  return;
}

/* Receive a release_started message, which indicates that a sound has entered
 * its release stage.  */
void
sound_release_started (const gchar * sound_name, GApplication * app)
{
  GList *sound_effect_list;
  struct sound_info *sound_effect = NULL;
  gboolean sound_effect_found;

  /* Search through the sound effects for the one with this name.  */
  sound_effect_list = sep_get_sound_list (app);
  sound_effect_found = FALSE;
  while (sound_effect_list != NULL)
    {
      sound_effect = sound_effect_list->data;
      if (g_strcmp0 (sound_effect->name, sound_name) == 0)
        {
          sound_effect_found = TRUE;
          break;
        }
      sound_effect_list = sound_effect_list->next;
    }

  /* If there isn't one, ignore the termination message.  */
  if (!sound_effect_found)
    return;

  /* Remember that the sound is in its release stage.  */
  sound_effect->release_has_started = TRUE;

  /* Let the internal sequencer handle it.  */
  sequence_sound_release_started (sound_effect, app);

  return;
}

/* The Pause button was pushed.  */
void
sound_button_pause (GApplication * app)
{
  GList *sound_list;
  GList *l;
  struct sound_info *sound_data;
  GstBin *bin_element;
  GstEvent *event;
  GstStructure *structure;

  sound_list = sep_get_sound_list (app);

  /* Go through the non-disabled sounds, sending each a pause command.  */
  for (l = sound_list; l != NULL; l = l->next)
    {
      sound_data = l->data;
      if (!sound_data->disabled)
        {
          bin_element = sound_data->sound_control;

          /* Send a pause message to the bin.  The looper element will stop
           * advancing its pointer, sending silence instead, and the envelope
           * element will stop advancing through its timeline.  */
          structure = gst_structure_new_empty ((gchar *) "pause");
          event = gst_event_new_custom (GST_EVENT_CUSTOM_UPSTREAM, structure);
          gst_element_send_event (GST_ELEMENT (bin_element), event);
        }

    }
  return;
}

/* The Continue button was pushed.  */
void
sound_button_continue (GApplication * app)
{
  GList *sound_list;
  GList *l;
  struct sound_info *sound_data;
  GstBin *bin_element;
  GstEvent *event;
  GstStructure *structure;

  sound_list = sep_get_sound_list (app);

  /* Go through the non-disabled sounds, sending each a continue command.  */
  for (l = sound_list; l != NULL; l = l->next)
    {
      sound_data = l->data;
      if (!sound_data->disabled)
        {
          bin_element = sound_data->sound_control;

          /* Send a continue message to the bin.  The looper element will 
           * return to advancing its pointer, and the envelope element will 
           * return to advancing through its timeline.  */
          structure = gst_structure_new_empty ((gchar *) "continue");
          event = gst_event_new_custom (GST_EVENT_CUSTOM_UPSTREAM, structure);
          gst_element_send_event (GST_ELEMENT (bin_element), event);
        }

    }

  return;
}
