/*
 * sound_effects_player.h
 *
 * Copyright © 2015 by John Sauter <John_Sauter@systemeyescomputerstore.com>
 * 
 * Sound_effects_player is free software: you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * Sound_effects_player is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _SOUND_EFFECTS_PLAYER_H_
#define _SOUND_EFFECTS_PLAYER_H_

#include <gtk/gtk.h>
#include <gst/gst.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

G_BEGIN_DECLS
#define SOUND_EFFECTS_PLAYER_TYPE_APPLICATION (sound_effects_player_get_type ())
#define SOUND_EFFECTS_PLAYER_APPLICATION(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), SOUND_EFFECTS_PLAYER_TYPE_APPLICATION, Sound_Effects_Player))
#define SOUND_EFFECTS_PLAYER_APPLICATION_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), SOUND_EFFECTS_PLAYER_TYPE_APPLICATION, Sound_Effects_PlayerClass))
#define SOUND_EFFECTS_PLAYER_IS_APPLICATION(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SOUND_EFFECTS_PLAYER_TYPE_APPLICATION))
#define SOUND_EFFECTS_PLAYER_IS_APPLICATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SOUND_EFFECTS_PLAYER_TYPE_APPLICATION))
#define SOUND_EFFECTS_PLAYER_APPLICATION_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), SOUND_EFFECTS_PLAYER_TYPE_APPLICATION, Sound_Effects_PlayerClass))
typedef struct _Sound_Effects_PlayerClass Sound_Effects_PlayerClass;
typedef struct _Sound_Effects_Player Sound_Effects_Player;
typedef struct _Sound_Effects_PlayerPrivate Sound_Effects_PlayerPrivate;

struct _Sound_Effects_PlayerClass
{
  GtkApplicationClass parent_class;
};

struct _Sound_Effects_Player
{
  GtkApplication parent_instance;

  Sound_Effects_PlayerPrivate *priv;

};

GType
sound_effects_player_get_type (void) G_GNUC_CONST; Sound_Effects_Player *sound_effects_player_new (void);

/* Information about a sound effect we might play */
struct sound_effect_str
{
  GtkWidget *cluster;      /* The cluster the sound is in, if any */
  gchar *file_name;        /* The name of the file holding the sound */
  GstBin *sound_control;   /* The Gstreamer bin for this sound effect */
  gint cluster_number;     /* The number of the cluster the sound is in */
};

/* Callbacks */

/* Given a widget, get its pipeline. */
GstPipeline *sep_get_pipeline (GtkWidget * object);

/* Given a widget in a cluster, get its sound_effect structure. */
struct sound_effect_str *sep_get_sound_effect (GtkWidget * object);

/* Given the application, find the common area above the clusters. */
GtkWidget *sep_get_common_area (GApplication * app);

/* Given the application, find the network information. */
void *sep_get_network_data (GApplication * app);

/* Given the application, find the parser information. */
void *sep_get_parse_info (GApplication * app);

/* Given the application, find the top-level window. */
GtkWindow *sep_get_top_window (GApplication * app);

/* Given the application, find the project file. */
xmlDocPtr sep_get_project_file (GApplication * app);

/* Given the application, remember the project file. */
void sep_set_project_file (xmlDocPtr project_file, GApplication * app);

/* Given the application, find the name of the project file. */
gchar *sep_get_project_filename (GApplication * app);

/* Given the application, remember the name of the project file. */
void sep_set_project_filename (gchar * project_filename, GApplication * app);

/* Given the application, find the path to the user interface files. */
gchar *sep_get_ui_path (GApplication * app);

/* Start playing the sound in a specified cluster. */
void sep_start_cluster (int cluster_no, GApplication * app);

/* Stop playing the sound in a specified cluster. */
void sep_stop_cluster (int cluster_no, GApplication * app);

G_END_DECLS
#endif /* _SOUND_EFFECTS_PLAYER_H_ */
