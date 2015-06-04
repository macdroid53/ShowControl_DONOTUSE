/*
 * sound_subroutines.c
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
#include "sound_subroutines.h"
#include "sound_structure.h"
#include "sound_effects_player.h"
#include "gstreamer_subroutines.h"
#include "button_subroutines.h"

/* Subroutines for processing sounds.  */

/* Initialize the sound system.  We have already read an XML file
 * containing sound definitions and put the results in the sound list.  */
GstPipeline *
sound_init (GApplication * app)
{
  GstPipeline *pipeline_element;
  GList *sound_list;
  gint i, sound_count;
  GList *l;
  struct sound_info *sound_data;
  GList *children_list;
  const char *child_name;
  GtkLabel *title_label;

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
   * standard items in it.  */
  if (sound_count > 0)
    {
      pipeline_element = gstreamer_init (sound_count, app);

      /* Create a gstreamer bin for each enabled sound effect and place it in
       * the gstreamer pipeline.  */
      i = 0;
      for (l = sound_list; l != NULL; l = l->next)
        {
          sound_data = l->data;
          if (!sound_data->disabled)
            {
              sound_data->sound_control =
                gstreamer_create_bin (sound_data, i, pipeline_element, app);

              /* Since we do not have an internal sequencer, place the sound
               * in a cluster.  */
              sound_data->cluster = sep_get_cluster (i, app);
              sound_data->cluster_number = i;

              /* Set the name of the sound in the cluster.  */
              title_label = NULL;
              children_list =
                gtk_container_get_children (GTK_CONTAINER
                                            (sound_data->cluster));
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
                  gtk_label_set_label (title_label, sound_data->name);
                }
              i = i + 1;
            }
        }

      /* Now that all of the sound effects are processed, complete the
       * gstreamer pipeline.  */
      gstreamer_complete_pipeline (pipeline_element, app);
    }
  else
    {
      /* If we have no sounds, we have no pipeline.  */
      pipeline_element = NULL;
    }

  return pipeline_element;
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

/* Start playing the sound in a specified cluster. */
void
sound_cluster_start (int cluster_no, GApplication * app)
{
  GList *sound_effect_list;
  GtkWidget *cluster_widget = NULL;
  struct sound_info *sound_effect = NULL;
  gboolean sound_effect_found;
  GList *children_list;
  GtkButton *start_button = NULL;
  const gchar *child_name;

  /* Search through the sound effects for the one attached
   * to this cluster. */
  sound_effect_list = sep_get_sound_list (app);
  sound_effect_found = FALSE;
  while (sound_effect_list != NULL)
    {
      sound_effect = sound_effect_list->data;
      if ((sound_effect->cluster != NULL)
          && (sound_effect->cluster_number == cluster_no))
        {
          sound_effect_found = TRUE;
          break;
        }
      sound_effect_list = sound_effect_list->next;
    }
  if (sound_effect_found)
    {
      cluster_widget = sound_effect->cluster;
      /* Find the start button in the cluster. */
      children_list =
        gtk_container_get_children (GTK_CONTAINER (cluster_widget));
      while (children_list != NULL)
        {
          child_name = gtk_widget_get_name (children_list->data);
          if (g_ascii_strcasecmp (child_name, "start_button") == 0)
            {
              start_button = children_list->data;
              break;
            }
          children_list = children_list->next;
        }
      g_list_free (children_list);

      /* Invoke the start button, so the sound starts to play and
       * and the button appearance is updated. */
      button_start_clicked (start_button, cluster_widget);
    }

  return;
}

/* Stop playing the sound in a specified cluster. */
void
sound_cluster_stop (int cluster_no, GApplication * app)
{
  GList *sound_effect_list;
  GtkWidget *cluster_widget = NULL;
  struct sound_info *sound_effect = NULL;
  gboolean sound_effect_found;
  GList *children_list;
  GtkButton *stop_button = NULL;
  const gchar *child_name;

  /* Search through the sound effects for the one attached
   * to this cluster. */
  sound_effect_list = sep_get_sound_list (app);
  sound_effect_found = FALSE;
  while (sound_effect_list != NULL)
    {
      sound_effect = sound_effect_list->data;
      if ((sound_effect->cluster != NULL)
          && (sound_effect->cluster_number == cluster_no))
        {
          sound_effect_found = TRUE;
          break;
        }
      sound_effect_list = sound_effect_list->next;
    }
  if (sound_effect_found)
    {
      cluster_widget = sound_effect->cluster;
      /* Find the stop button in the cluster. */
      children_list =
        gtk_container_get_children (GTK_CONTAINER (cluster_widget));
      while (children_list != NULL)
        {
          child_name = gtk_widget_get_name (children_list->data);
          if (g_ascii_strcasecmp (child_name, "stop_button") == 0)
            {
              stop_button = children_list->data;
              break;
            }
          children_list = children_list->next;
        }
      g_list_free (children_list);

      /* Invoke the stop button, so the sound stops playing and
       * and the appearance of the start button is updated. */
      button_stop_clicked (stop_button, cluster_widget);
    }

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

  /* Send a start message to the bin.  It will be routed to the source, and
   * flow from there downstream through the looper and envelope.  
   * The looper element will start sending its local buffer,
   * and the envelope element will start shaping the volume.  */
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
   * We should get a call to sound_terminated shortly.  */
  structure = gst_structure_new_empty ((gchar *) "release");
  event = gst_event_new_custom (GST_EVENT_CUSTOM_UPSTREAM, structure);
  gst_element_send_event (GST_ELEMENT (bin_element), event);

  return;
}

/* Receive a completed message, which indicates that a sound has entered
 * its release stage.  */
void
sound_completed (const gchar * sound_name, GApplication * app)
{
  GList *sound_effect_list;
  struct sound_info *sound_effect = NULL;
  gboolean sound_effect_found;

  g_print ("sound %s completed.\n", sound_name);

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
  if (sound_effect_found)
    {
      /* Set the Playing label on the button back to Start.  */
      button_reset_cluster (sound_effect, app);
    }

  /* More code will be needed for the sequencer.  */
  return;
}

/* Receive a terminated message, which indicates that a sound has entered
 * its release stage due to an external event.  */
void
sound_terminated (const gchar * sound_name, GApplication * app)
{
  GList *sound_effect_list;
  struct sound_info *sound_effect = NULL;
  gboolean sound_effect_found;

  g_print ("sound %s terminated.\n", sound_name);

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
  if (sound_effect_found)
    {
      /* Set the Playing label on the button back to Start.  */
      button_reset_cluster (sound_effect, app);
    }

  /* More code will be needed for the sequencer.  */
  return;
}
