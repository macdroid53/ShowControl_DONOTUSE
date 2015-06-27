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

/* Callbacks */

/* The gstreamer pipeline has completed initialization; we can show
 * the top-level window now.  */
void sep_gstreamer_ready (GApplication *app);

/* Create the gstreamer pipeline by reading an XML file.  */
void sep_create_pipeline (gchar * filename, GApplication *app);

/* Find the gstreamer pipeline.  */
GstPipeline *sep_get_pipeline_from_app (GApplication * app);

/* Given a widget, get the app.  */
GApplication *sep_get_application_from_widget (GtkWidget * object);

/* Given a widget within a cluster, find the cluster.  */
GtkWidget *sep_get_cluster_from_widget (GtkWidget *the_widget);

/* Given a widget in a cluster, get its sound_effect structure. */
struct sound_info *sep_get_sound_effect (GtkWidget * object);

/* Given a cluster number, get the cluster.  */
GtkWidget *sep_get_cluster_from_number (guint cluster_number,
					GApplication * app);

/* Given a cluster, get its number.  */
guint sep_get_cluster_number (GtkWidget *cluster_widget);

/* Find the common area above the clusters. */
GtkWidget *sep_get_common_area (GApplication * app);

/* Find the network information. */
void *sep_get_network_data (GApplication * app);

/* Find the network messages parser information. */
void *sep_get_parse_net_data (GApplication * app);

/* Find the top-level window. */
GtkWindow *sep_get_top_window (GApplication * app);

/* Find the status bar.  */
GtkStatusbar *sep_get_status_bar (GApplication * app);

/* Find the context ID.  */
guint sep_get_context_id (GApplication * app);

/* Find the project file. */
xmlDocPtr sep_get_project_file (GApplication * app);

/* Remember the project file. */
void sep_set_project_file (xmlDocPtr project_file, GApplication * app);

/* Find the name of the project file. */
gchar *sep_get_project_filename (GApplication * app);

/* Remember the name of the project file. */
void sep_set_project_filename (gchar * project_filename, GApplication * app);

/* Find the path to the user interface files. */
gchar *sep_get_ui_path (GApplication * app);

/* Find the list of sound effects.  */
GList *sep_get_sound_list (GApplication *app);

/* Set the list of sound effects.  */
void sep_set_sound_list (GList *sound_list, GApplication *app);

/* Find the sequence information.  */
void *sep_get_sequence_data (GApplication *app);

G_END_DECLS
#endif /* _SOUND_EFFECTS_PLAYER_H_ */
