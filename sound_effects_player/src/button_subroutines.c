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
#include "sound_effects_player.h"
#include "sound_subroutines.h"
#include "sound_structure.h"

/* The Start button has been pushed.  Turn the sound effect on. */
void
button_start_clicked (GtkButton * button, gpointer user_data)
{
  struct sound_info *sound_data;
  GApplication *app;

  sound_data = sep_get_sound_effect (user_data);
  app = sep_get_application_from_widget (user_data);

  /* If there is no sound connected to this button, do nothing.  */
  if (sound_data == NULL)
    return;

  /* Tell gstreamer to start playing the sound.  */
  sound_start_playing (sound_data, app);


  /* Change the text on the button from "Start" to "Playing". */
  gtk_button_set_label (button, "Playing");

  return;
}

/* The stop button has been pushed.  Turn the sound effect off. */
void
button_stop_clicked (GtkButton * button, gpointer user_data)
{
  struct sound_info *sound_data;
  GApplication *app;

  sound_data = sep_get_sound_effect (user_data);
  app = sep_get_application_from_widget (user_data);

  /* If there is no sound attached to this cluster, do nothing.  */
  if (sound_data == NULL)
    return;

  /* stop playing this sound.  */
  sound_stop_playing (sound_data, app);

  /* Reset the cluster appearance.  */
  button_reset_cluster (sound_data, app);

  return;
}

/* Reset the appearance of a cluster after its stop button has been
 * pushed or it has finished playing. */
void
button_reset_cluster (struct sound_info *sound_data, GApplication * app)
{
  GtkButton *start_button = NULL;
  GtkWidget *parent_container;
  GList *children_list = NULL;
  const gchar *child_name = NULL;

  /* Find the start button and set its text back to "Start". 
   * The start button will be a child of the cluster, and will be named
   * "start_button".  */
  parent_container = sound_data->cluster;
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
  struct sound_info *sound_data;
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
      sound_data = sep_get_sound_effect (user_data);
      if (sound_data == NULL)
        return;

      /* The sound_effect structure records where the Gstreamer bin is
       * for this sound effect.  That bin contains the volume control.
       */
      bin_element = sound_data->sound_control;
      volume_element = gstreamer_get_volume (bin_element);
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
  struct sound_info *sound_data;
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
      sound_data = sep_get_sound_effect (user_data);
      if (sound_data == NULL)
        return;

      /* The sound_effect structure records where the Gstreamer bin is
       * for this sound effect.  That bin contains the pan control.
       */
      bin_element = sound_data->sound_control;
      pan_element = gstreamer_get_pan (bin_element);
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
