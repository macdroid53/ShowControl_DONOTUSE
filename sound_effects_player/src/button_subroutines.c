/*
 * button_subroutines.c
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
#include "button_subroutines.h"
#include "gstreamer_subroutines.h"
#include "sound_effects_player.h"
#include "sound_subroutines.h"
#include "sound_structure.h"
#include "sequence_subroutines.h"

/* The Mute button has been toggled.  */
void
button_mute_toggled (GtkToggleButton * button, gpointer user_data)
{
  GApplication *app;
  gboolean button_state;
  GstPipeline *pipeline_element;
  GstElement *volume_element;
  GstElement *final_bin_element;

  app = sep_get_application_from_widget (user_data);
  button_state = gtk_toggle_button_get_active (button);
  pipeline_element = sep_get_pipeline_from_app (app);
  if (pipeline_element == NULL)
    return;

  /* Find the final bin.  */
  final_bin_element =
    gst_bin_get_by_name (GST_BIN (pipeline_element), (gchar *) "final");
  if (final_bin_element == NULL)
    return;

  /* Find the volume element in the final bin */
  volume_element = gstreamer_get_volume (GST_BIN (final_bin_element));
  if (volume_element == NULL)
    return;

  /* Set the mute property of the volume element based on whether
   * the mute button has been activated or deactivated.  */
  g_object_set (volume_element, "mute", button_state, NULL);

  return;
}

/* The Pause button has been pushed.  */
void
button_pause_clicked (GtkButton * button, gpointer user_data)
{
  GApplication *app;

  app = sep_get_application_from_widget (user_data);
  sound_button_pause (app);

  return;
}

/* The Continue button has been pushed.  */
void
button_continue_clicked (GtkButton * button, gpointer user_data)
{
  GApplication *app;

  app = sep_get_application_from_widget (user_data);
  sound_button_continue (app);

  return;
}

/* The Play button has been pushed.  */
void
button_play_clicked (GtkButton * button, gpointer user_data)
{
  GApplication *app;

  /* Let the internal sequencer handle it.  */
  app = sep_get_application_from_widget (user_data);
  sequence_button_play (app);

  return;
}

/* The Start button in a cluster has been pushed.  */
void
button_start_clicked (GtkButton * button, gpointer user_data)
{
  GApplication *app;
  GtkWidget *cluster_widget;
  guint cluster_number;

  /* Let the internal sequencer handle it.  */
  app = sep_get_application_from_widget (user_data);
  cluster_widget = sep_get_cluster_from_widget (user_data);
  cluster_number = sep_get_cluster_number (cluster_widget);
  sequence_cluster_start (cluster_number, app);

  return;
}

/* The Stop button in a cluster has been pushed.  */
void
button_stop_clicked (GtkButton * button, gpointer user_data)
{
  GApplication *app;
  GtkWidget *cluster_widget;
  guint cluster_number;

  /* Let the internal sequencer handle it.  */
  app = sep_get_application_from_widget (user_data);
  cluster_widget = sep_get_cluster_from_widget (user_data);
  cluster_number = sep_get_cluster_number (cluster_widget);
  sequence_cluster_stop (cluster_number, app);

  return;
}

/* Show that the Start button has been pushed.  */
void
button_set_cluster_playing (struct sound_info *sound_data, GApplication * app)
{
  GtkButton *start_button = NULL;
  GtkWidget *parent_container;
  GList *children_list = NULL;
  const gchar *child_name = NULL;

  /* Find the start button and set its text to "Playing...". 
   * The start button will be a child of the cluster, and will be named
   * "start_button".  */
  parent_container = sound_data->cluster_widget;

  /* It is possible, though unlikely, that the sound will no longer
   * be in a cluster.  */
  if (parent_container != NULL)
    {
      children_list =
        gtk_container_get_children (GTK_CONTAINER (parent_container));
      while (children_list != NULL)
        {
          child_name = gtk_widget_get_name (children_list->data);
          if (g_strcmp0 (child_name, "start_button") == 0)
            {
              start_button = children_list->data;
              break;
            }
          children_list = children_list->next;
        }
      g_list_free (children_list);
      gtk_button_set_label (start_button, "Playing...");
    }

  return;
}

/* Show that the release stage of a sound is running.  */
void
button_set_cluster_releasing (struct sound_info *sound_data,
                              GApplication * app)
{
  GtkButton *start_button = NULL;
  GtkWidget *parent_container;
  GList *children_list = NULL;
  const gchar *child_name = NULL;

  /* Find the start button and set its text to "Releasing...". 
   * The start button will be a child of the cluster, and will be named
   * "start_button".  */
  parent_container = sound_data->cluster_widget;

  /* It is possible, though unlikely, that the sound will no longer
   * be in a cluster.  */
  if (parent_container != NULL)
    {
      children_list =
        gtk_container_get_children (GTK_CONTAINER (parent_container));
      while (children_list != NULL)
        {
          child_name = gtk_widget_get_name (children_list->data);
          if (g_strcmp0 (child_name, "start_button") == 0)
            {
              start_button = children_list->data;
              break;
            }
          children_list = children_list->next;
        }
      g_list_free (children_list);
      gtk_button_set_label (start_button, "Releasing...");
    }

  return;
}

/* Reset the appearance of a cluster after its sound has finished playing. */
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
  parent_container = sound_data->cluster_widget;

  /* It is possible, though unlikely, that the sound will no longer
   * be in a cluster.  */
  if (parent_container != NULL)
    {
      children_list =
        gtk_container_get_children (GTK_CONTAINER (parent_container));
      while (children_list != NULL)
        {
          child_name = gtk_widget_get_name (children_list->data);
          if (g_strcmp0 (child_name, "start_button") == 0)
            {
              start_button = children_list->data;
              break;
            }
          children_list = children_list->next;
        }
      g_list_free (children_list);
      gtk_button_set_label (start_button, "Start");
    }

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
      if (g_strcmp0 (child_name, "volume_label") == 0)
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
      if (g_strcmp0 (child_name, "pan_label") == 0)
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
      /* The pan control may be omitted by the sound designer.  */
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
