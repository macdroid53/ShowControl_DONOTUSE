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
#include "sound_subroutines.h"
#include "sound_structure.h"
#include "sound_effects_player.h"
#include "button_subroutines.h"

/* Subroutines for processing sounds.  */

/* Initialize the sound system.  We have read and processed an XML file
 * containing sound definitions.  */
void
sound_init (GApplication * app)
{
  GList *sound_list;
  GList *l;
  struct sound_info *sound_effect;

  sound_list = sep_get_sound_list (app);

  for (l = sound_list; l != NULL; l = l->next)
    {
      sound_effect = l->data;
      g_print (sound_effect->name);
      g_print ("\n");
    }

  return;
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
       * and button appearance is updated. */
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
