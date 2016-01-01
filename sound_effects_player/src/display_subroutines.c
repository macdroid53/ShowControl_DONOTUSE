/*
 * display_subroutines.c
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
#include "display_subroutines.h"
#include "sound_effects_player.h"

/* Update the VU meter. */
void
display_update_vu_meter (gpointer * user_data, gint channel,
                         gdouble new_value, gdouble peak_dB, gdouble decay_dB)
{
  GtkWidget *common_area;
  GList *children_list;
  const gchar *child_name;
  GtkWidget *VU_meter = NULL;
  gint VU_level;
  gint64 VU_led_number;
  GtkLabel *VU_lamp;
  GtkBox *channel_row = NULL;
  gint64 channel_number;

  common_area = sep_get_common_area (G_APPLICATION (user_data));

  /* Find the VU meter in the common area. */
  children_list = gtk_container_get_children (GTK_CONTAINER (common_area));
  while (children_list != NULL)
    {
      child_name = gtk_widget_get_name (children_list->data);
      if (g_ascii_strcasecmp (child_name, "VU_meter") == 0)
        {
          VU_meter = children_list->data;
          break;
        }
      children_list = children_list->next;
    }
  g_list_free (children_list);

  if (VU_meter == NULL)
    return;

  /* Within the VU_meter box is a box for each channel.  Find the box
   * for this channel. */
  children_list = gtk_container_get_children (GTK_CONTAINER (VU_meter));
  while (children_list != NULL)
    {
      channel_row = GTK_BOX (children_list->data);
      child_name = gtk_widget_get_name (GTK_WIDGET (channel_row));
      channel_number = g_ascii_strtoll (child_name, NULL, 10);
      if (channel_number == channel)
        break;
      children_list = children_list->next;
    }
  g_list_free (children_list);
  if (channel_row == NULL)
    return;

  /* Within the VU_meter's channel box is a bunch of labels, each with a name
   * indicating its position on the line.  Light those to the left
   * of the desired value. */
  VU_level = new_value * 50.0;
  if (VU_level > 50)
    VU_level = 50;
  if (VU_level < 1)
    VU_level = 0;
  children_list = gtk_container_get_children (GTK_CONTAINER (channel_row));
  while (children_list != NULL)
    {
      VU_lamp = GTK_LABEL (children_list->data);
      child_name = gtk_widget_get_name (GTK_WIDGET (VU_lamp));
      VU_led_number = g_ascii_strtoll (child_name, NULL, 10);
      if (VU_led_number < VU_level)
        {
          gtk_label_set_text (VU_lamp, "*");
        }
      else
        {
          gtk_label_set_text (VU_lamp, " ");
        }
      children_list = children_list->next;
    }
  g_list_free (children_list);

  return;
}

/* Show the user a message.  The return value is a message ID, which
 * can be used to remove the message.  */
guint
display_show_message (gchar * message_text, GApplication * app)
{
  GtkStatusbar *status_bar;
  guint context_id;
  guint message_id;

  /* Find the GUI's status display area.  */
  status_bar = sep_get_status_bar (app);

  /* Use the regular context for messages.  */
  context_id = sep_get_context_id (app);

  /* Show the message.  */
  message_id = gtk_statusbar_push (status_bar, context_id, message_text);

  return message_id;
}

/* Remove a previously-displayed message.  */
void
display_remove_message (guint message_id, GApplication * app)
{
  GtkStatusbar *status_bar;
  guint context_id;

  /* Find the GUI's status display area.  */
  status_bar = sep_get_status_bar (app);

  /* Get the message context ID.  */
  context_id = sep_get_context_id (app);

  /* Remove the specified message.  */
  gtk_statusbar_remove (status_bar, context_id, message_id);

  return;
}

/* Display a message to the operator.  */
void
display_set_operator_text (gchar * text_to_display, GApplication *app)
{
  GtkLabel *text_label;
  
  /* Find the GUI's operator text area.  */
  text_label = sep_get_operator_text (app);

  /* Set the text.  */
  gtk_label_set_text (text_label, text_to_display);

  return;
}

/* Erase any operator message.  */
void
display_clear_operator_text (GApplication *app)
{
  GtkLabel *text_label;

  /*Find the GUI's operator text area.  */
  text_label = sep_get_operator_text (app);

  /* Clear the text.  */
  gtk_label_set_text (text_label, (gchar *) "");

  return;
}
