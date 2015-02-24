/*
 * button_subroutines.c
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
#include "button_subroutines.h"
#include "gstreamer_subroutines.h"
#include "play_sound.h"

/* The Start button has been pushed.  Turn the sound effect on. */
void
button_start_clicked (GtkButton * button, gpointer user_data)
{
  struct sound_effect_str *sound_effect;
  GstBin *bin_element;
  GstElement *volume_element;
  GstPipeline *pipeline_element;

  sound_effect = play_sound_get_sound_effect (user_data);
  if (sound_effect == NULL)
    return;
  bin_element = sound_effect->sound_control;
  volume_element = gstreamer_find_volume (bin_element);
  /* Unmute the bin to hear its sound. */
  g_object_set (volume_element, "mute", FALSE, NULL);

  /* Change the text on the button from "Start" to "Playing". */
  gtk_button_set_label (button, "Playing");

  /* For debugging, output an annotated, graphical representation
   * of the pipeline.
   */
  pipeline_element = play_sound_get_pipeline (user_data);
  gstreamer_dump_pipeline (pipeline_element);

  return;
}

/* The stop button has been pushed.  Turn the sound effect off. */
void
button_stop_clicked (GtkButton * button, gpointer user_data)
{
  struct sound_effect_str *sound_effect;
  GstBin *bin_element;
  GstElement *volume_element;
  GstPipeline *pipeline_element;
  GtkButton *start_button = NULL;
  GtkWidget *parent_container;
  GList *children_list = NULL;
  const gchar *child_name = NULL;

  sound_effect = play_sound_get_sound_effect (user_data);
  if (sound_effect == NULL)
    return;
  bin_element = sound_effect->sound_control;
  volume_element = gstreamer_find_volume (bin_element);
  /* Mute the volume, so we no longer hear it. */
  g_object_set (volume_element, "mute", TRUE, NULL);

  /* Find the start button and set its text back to "Start". 
   * The start button will be in the stop button's parent container,
   * and will be named "start_button". */
  parent_container = gtk_widget_get_parent (GTK_WIDGET (button));
  children_list =
    gtk_container_get_children (GTK_CONTAINER (parent_container));
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
  gtk_button_set_label (start_button, "Start");

  /* For debugging, output an annotated, graphical representation
   * of the pipeline.
   */
  pipeline_element = play_sound_get_pipeline (user_data);
  gstreamer_dump_pipeline (pipeline_element);

  return;
}

/* The volume slider has been moved.  Update the volume and the display. 
 * The user data is the widget being controlled. */
void
button_volume_changed (GtkButton * button, gpointer user_data)
{
  GtkLabel *volume_label = NULL;
  GtkWidget *parent_container;
  GList *children_list = NULL;
  const gchar *child_name = NULL;
  struct sound_effect_str *sound_effect;
  GstBin *bin_element;
  GstElement *volume_element;
  gdouble new_value;
  gchar *value_string;

  /* Find the volume label associated with this volume widget.
   * It will be a child of this widget's parent. */
  parent_container = gtk_widget_get_parent (GTK_WIDGET (button));
  children_list =
    gtk_container_get_children (GTK_CONTAINER (parent_container));
  while (children_list != NULL)
    {
      child_name = gtk_widget_get_name (children_list->data);
      if (g_ascii_strcasecmp (child_name, "volume_label") == 0)
        {
          volume_label = children_list->data;
          break;
        }
      children_list = children_list->next;
    }
  g_list_free (children_list);

  if (volume_label != NULL)
    {
      /* There should be a sound effect associated with this cluster.
       * If there isn't, do nothing. */
      sound_effect = play_sound_get_sound_effect (user_data);
      if (sound_effect == NULL)
        return;

      /* The sound_effect structure records where the Gstreamer bin is
       * for this sound effect.  That bin contains the volume control.
       */
      bin_element = sound_effect->sound_control;
      volume_element = gst_bin_get_by_name (GST_BIN (bin_element), "volume");
      if (volume_element == NULL)
        return;

      new_value = gtk_scale_button_get_value (GTK_SCALE_BUTTON (button));
      /* Set the volume of the sound. */
      g_object_set (volume_element, "volume", new_value, NULL);

      /* Update the text in the volume label. */
      value_string = g_strdup_printf ("Vol%4.0f%%", new_value * 100.0);
      gtk_label_set_text (volume_label, value_string);
      g_free (value_string);

    }

  return;
}

/* The pan slider has been moved.  Update the pan and display. */
void
button_pan_changed (GtkButton * button, gpointer user_data)
{
  GtkLabel *pan_label = NULL;
  GtkWidget *parent_container;
  GList *children_list = NULL;
  const gchar *child_name = NULL;
  struct sound_effect_str *sound_effect;
  GstBin *bin_element;
  GstElement *pan_element;
  gdouble new_value;
  gchar *value_string;

  /* Find the pan label associated with this pan widget.
   * It will be a child of this widget's parent. */
  parent_container = gtk_widget_get_parent (GTK_WIDGET (button));
  children_list =
    gtk_container_get_children (GTK_CONTAINER (parent_container));
  while (children_list != NULL)
    {
      child_name = gtk_widget_get_name (children_list->data);
      if (g_ascii_strcasecmp (child_name, "pan_label") == 0)
        {
          pan_label = children_list->data;
          break;
        }
      children_list = children_list->next;
    }
  g_list_free (children_list);

  if (pan_label != NULL)
    {
      /* There should be a sound effect associated with this cluster.
       * If there isn't, do nothing. */
      sound_effect = play_sound_get_sound_effect (user_data);
      if (sound_effect == NULL)
        return;

      /* The sound_effect structure records where the Gstreamer bin is
       * for this sound effect.  That bin contains the pan control.
       */
      bin_element = sound_effect->sound_control;
      pan_element = gst_bin_get_by_name (GST_BIN (bin_element), "pan");
      if (pan_element == NULL)
        return;

      new_value = gtk_scale_button_get_value (GTK_SCALE_BUTTON (button));
      /* Set the panorama position of the sound. */
      new_value = (new_value - 50.0) / 50.0;
      g_object_set (pan_element, "panorama", new_value, NULL);

      /* Update the text of the pan label.  0.0 corresponds to Center, 
       * negative numbers to left, and positive numbers to right. */
      if (new_value == 0.0)
        gtk_label_set_text (pan_label, "Center");
      else
        {
          if (new_value < 0.0)
            value_string =
              g_strdup_printf ("Left %4.0f%%", -(new_value * 100.0));
          else
            value_string =
              g_strdup_printf ("Right%4.0f%%", new_value * 100.0);
          gtk_label_set_text (pan_label, value_string);
          g_free (value_string);
        }
    }

  return;
}
