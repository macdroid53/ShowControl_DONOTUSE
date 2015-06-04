/*
 * sound_effects_player.c
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
#include <glib/gi18n.h>
#include <libxml/xmlmemory.h>
#include "sound_effects_player.h"
#include "sound_structure.h"
#include "gstreamer_subroutines.h"
#include "menu_subroutines.h"
#include "network_subroutines.h"
#include "parse_xml_subroutines.h"
#include "parse_net_subroutines.h"
#include "sound_subroutines.h"

G_DEFINE_TYPE (Sound_Effects_Player, sound_effects_player,
               GTK_TYPE_APPLICATION);

/* ANJUTA: Macro SOUND_EFFECTS_PLAYER_APPLICATION gets Sound_Effects_Player
 *  - DO NOT REMOVE */

/* The private data associated with the top-level window. */
struct _Sound_Effects_PlayerPrivate
{
  /* The Gstreamer pipeline. */
  GstPipeline *gstreamer_pipeline;

  /* A flag that is set by the Gstreamer startup process when it is
   * complete.  */
  gboolean gstreamer_ready;
  
  /* The top-level gtk window. */
  GtkWindow *top_window;

  /* A flag that is set to indicate that we have told GTK to start
   * showing the top-level window.  */
  gboolean windows_showing;
  
  /* The common area, needed for updating the display asynchronously. */
  GtkWidget *common_area;

  /* The list of sounds we can make.  Each item of the GList points
   * to a sound_info structure. */
  GList *sound_list;

  /* The list of clusters that might contain sound effects. */
  GList *clusters;

  /* The persistent network information. */
  void *network_data;

  /* The persistent information for the network commands parser. */
  void *parse_net_data;

  /* The XML file that holds parameters for the program. */
  xmlDocPtr project_file;

  /* The name of that file, for use in Save and as the default file
   * name for Save As. */
  gchar *project_filename;

  /* The path to the user interface files. */
  gchar *ui_path;

  /* ANJUTA: Widgets declaration for sound_effects_player.ui - DO NOT REMOVE */
};

/* Create a new window loading a file. */
static void
sound_effects_player_new_window (GApplication * app, GFile * file)
{
  GtkWindow *top_window;
  GtkWidget *common_area;
  GtkBuilder *builder;
  GError *error = NULL;
  gint cluster_number;
  gchar *cluster_name;
  GtkWidget *cluster_widget;
  gchar *filename;
  gchar *local_filename;

  Sound_Effects_PlayerPrivate *priv =
    SOUND_EFFECTS_PLAYER_APPLICATION (app)->priv;

  /* Remember the path to the user interface files. */
  priv->ui_path = g_strdup (PACKAGE_DATA_DIR "/ui/");

  /* Load the main user interface definition from its file. */
  builder = gtk_builder_new ();
  filename = g_strconcat (priv->ui_path, "sound_effects_player.ui", NULL);
  if (!gtk_builder_add_from_file (builder, filename, &error))
    {
      g_critical ("Couldn't load builder file %s: %s", filename,
                  error->message);
      g_error_free (error);
    }

  /* Auto-connect signal handlers. */
  gtk_builder_connect_signals (builder, app);

  /* Get the top-level window object from the user interface file. */
  top_window =
    GTK_WINDOW (gtk_builder_get_object (builder, "top_level_window"));
  priv->top_window = top_window;
  if (top_window == NULL)
    {
      g_critical ("Widget \"top_level_window\" is missing in file %s.",
                  filename);
    }

  /* Also get the common area. */
  common_area = GTK_WIDGET (gtk_builder_get_object (builder, "common_area"));
  priv->common_area = GTK_WIDGET (common_area);
  if (!common_area)
    {
      g_critical ("Widget \"common_area\" is missing in file %s.", filename);
    }

  /* We are done with the name of the user interface file. */
  g_free (filename);

  /* Remember where the clusters are. Each cluster has a name identifying it. */
  priv->clusters = NULL;
  for (cluster_number = 0; cluster_number < 16; ++cluster_number)
    {
      cluster_name = g_strdup_printf ("cluster_%2.2d", cluster_number);
      cluster_widget =
        GTK_WIDGET (gtk_builder_get_object (builder, cluster_name));
      if (cluster_widget != NULL)
        {
          priv->clusters = g_list_prepend (priv->clusters, cluster_widget);
        }
      g_free (cluster_name);
    }

  /* ANJUTA: Widgets initialization for sound_effects_player.ui 
   * - DO NOT REMOVE */

  g_object_unref (builder);

  gtk_window_set_application (top_window, GTK_APPLICATION (app));

  /* If the invocation of sound_effects_player included a parameter,
   * that parameter is the name of the project file to load before
   * starting the user interface.  */
  if (file != NULL)
    {
      priv->project_filename = g_file_get_parse_name (file);
    }
  else
    priv->project_filename = NULL;

  /* Set up the menu. */
  filename = g_strconcat (priv->ui_path, "app-menu.ui", NULL);
  menu_init (app, filename);
  g_free (filename);

  /* Set up the remainder of the private data. */
  priv->gstreamer_pipeline = NULL;
  priv->gstreamer_ready = FALSE;
  priv->sound_list = NULL;

  /* Initialize the network message parser. */
  priv->parse_net_data = parse_net_init (app);

  /* Listen for network messages. */
  priv->network_data = network_init (app);

  /* If we have a parameter, it is the project XML file to read for our sounds.
   * If we don't, the user will read a project XML file using the menu.  */
  if (priv->project_filename != NULL)
    {
      local_filename = g_strdup (priv->project_filename);
      parse_xml_read_project_file (local_filename, app);
      priv->gstreamer_pipeline = sound_init (app);
    }

  /* If we have a gstreamer pipeline but it has not completed its
   * initialization, don't display the window.  It will be displayed
   * when the gstreamer pipeline is ready.  */

  if ((priv->gstreamer_pipeline == NULL) || (priv->gstreamer_ready))
    {
      /* The display is initialized; time to show it. */
      gtk_widget_show_all (GTK_WIDGET (top_window));
      priv->windows_showing = TRUE;
    }
}

/* GApplication implementation */
static void
sound_effects_player_activate (GApplication * application)
{
  sound_effects_player_new_window (application, NULL);
}

static void
sound_effects_player_open (GApplication * application, GFile ** files,
                           gint n_files, const gchar * hint)
{
  gint i;

  for (i = 0; i < n_files; i++)
    sound_effects_player_new_window (application, files[i]);
}

static void
sound_effects_player_init (Sound_Effects_Player * object)
{
  object->priv =
    G_TYPE_INSTANCE_GET_PRIVATE (object,
                                 SOUND_EFFECTS_PLAYER_TYPE_APPLICATION,
                                 Sound_Effects_PlayerPrivate);
}

static void
sound_effects_player_finalize (GObject * object)
{
  GList *sound_effect_list;
  GList *next_sound_effect;
  struct sound_info *sound_effect;
  Sound_Effects_Player *self = (Sound_Effects_Player *) object;

  /* Deallocate the gstreamer pipeline.  */
  if (self->priv->gstreamer_pipeline != NULL)
    {
      g_object_unref (self->priv->gstreamer_pipeline);
      self->priv->gstreamer_pipeline = NULL;
    }

  /* Deallocate the list of sound effects. */
  sound_effect_list = self->priv->sound_list;

  while (sound_effect_list != NULL)
    {
      sound_effect = sound_effect_list->data;
      next_sound_effect = sound_effect_list->next;
      g_free (sound_effect->name);
      g_free (sound_effect->wav_file_name);
      g_free (sound_effect->wav_file_name_full);
      g_free (sound_effect->OSC_name);
      g_free (sound_effect->function_key);
      g_free (sound_effect);
      self->priv->sound_list =
        g_list_delete_link (self->priv->sound_list, sound_effect_list);
      sound_effect_list = next_sound_effect;
    }

  G_OBJECT_CLASS (sound_effects_player_parent_class)->finalize (object);
}

static void
sound_effects_player_class_init (Sound_Effects_PlayerClass * klass)
{
  G_APPLICATION_CLASS (klass)->activate = sound_effects_player_activate;
  G_APPLICATION_CLASS (klass)->open = sound_effects_player_open;

  g_type_class_add_private (klass, sizeof (Sound_Effects_PlayerPrivate));

  G_OBJECT_CLASS (klass)->finalize = sound_effects_player_finalize;
}

Sound_Effects_Player *
sound_effects_player_new (void)
{
  return g_object_new (sound_effects_player_get_type (), "application-id",
                       "org.gnome.show_control.sound_effects_player", "flags",
                       G_APPLICATION_HANDLES_OPEN, NULL);
}

/* Callbacks from other modules.  The names of the callbacks are prefixed
 * with sep_ rather than sound_effects_player_ for readability. */

/* Display the gtk top-level window.  This is called when the gstreamer
 * pipeline has completed initialization.  */
void
sep_gstreamer_ready (GApplication *app)
{
  Sound_Effects_PlayerPrivate *priv =
    SOUND_EFFECTS_PLAYER_APPLICATION (app)->priv;

  priv->gstreamer_ready = TRUE;
  if (!priv->windows_showing)
    {
      /* The gstreamer pipeline is ready, time to show the top-level window. */
      gtk_widget_show_all (GTK_WIDGET (priv->top_window));
      priv->windows_showing = TRUE;
    }

  return;
}

/* Create the gstreamer pipeline by reading an XML file.  */
void
sep_create_pipeline (gchar * filename, GApplication *app)
{
  Sound_Effects_PlayerPrivate *priv =
    SOUND_EFFECTS_PLAYER_APPLICATION (app)->priv;
  gchar *local_filename;

  local_filename = g_strdup (filename);
  parse_xml_read_project_file (local_filename, app);
  priv->gstreamer_pipeline = sound_init (app);

  return;
}

/* Find the gstreamer pipeline.  */
GstPipeline *
sep_get_pipeline_from_app (GApplication * app)
{
  Sound_Effects_PlayerPrivate *priv =
    SOUND_EFFECTS_PLAYER_APPLICATION (app)->priv;
  GstPipeline *pipeline_element;

  pipeline_element = priv->gstreamer_pipeline;

  return (pipeline_element);
}

/* Find the application, given any widget in the application.  */
GApplication *
sep_get_application_from_widget (GtkWidget * object)
{
  GtkWidget *toplevel_widget;
  GtkWindow *toplevel_window;
  GtkApplication *gtk_app;
  GApplication *app;

  /* Find the top-level window.  */
  toplevel_widget = gtk_widget_get_toplevel (object);
  toplevel_window = GTK_WINDOW (toplevel_widget);
  /* The top level window knows where to find the application.  */
  gtk_app = gtk_window_get_application (toplevel_window);
  app = (GApplication *) gtk_app;
  return (app);
}

/* Find the sound effect information corresponding to a cluster, 
 * given a widget inside that cluster. Return NULL if
 * the cluster is not running a sound effect. */
struct sound_info *
sep_get_sound_effect (GtkWidget * object)
{
  GtkWidget *this_object;
  const gchar *widget_name;
  GtkWidget *cluster_widget = NULL;
  GtkWidget *toplevel_widget;
  GtkWindow *toplevel_window;
  GtkApplication *app;
  Sound_Effects_Player *self;
  Sound_Effects_PlayerPrivate *priv;
  GList *sound_effect_list;
  struct sound_info *sound_effect = NULL;
  gboolean sound_effect_found;

  /* Work up from the given widget until we find one whose name starts
   * with "cluster_". */

  this_object = object;
  do
    {
      widget_name = gtk_widget_get_name (this_object);
      if (g_str_has_prefix (widget_name, "cluster_"))
        {
          cluster_widget = this_object;
          break;
        }
      this_object = gtk_widget_get_parent (this_object);
    }
  while (this_object != NULL);

  /* Find the application's private data, where the sound effects
   * information is kept.  First we find the top-level window, 
   * which has the private data. */
  toplevel_widget = gtk_widget_get_toplevel (object);
  toplevel_window = GTK_WINDOW (toplevel_widget);
  /* Work through the pointer structure to the private data. */
  app = gtk_window_get_application (toplevel_window);
  self = SOUND_EFFECTS_PLAYER_APPLICATION (app);
  priv = self->priv;

  /* Then we search through the sound effects for the one attached
   * to this cluster. */
  sound_effect_list = priv->sound_list;
  sound_effect_found = FALSE;
  while (sound_effect_list != NULL)
    {
      sound_effect = sound_effect_list->data;
      if (sound_effect->cluster == cluster_widget)
        {
          sound_effect_found = TRUE;
          break;
        }
      sound_effect_list = sound_effect_list->next;
    }
  if (sound_effect_found)
    return (sound_effect);

  return NULL;
}

/* Find a cluster, given its number.  */
GtkWidget *
sep_get_cluster (int cluster_number, GApplication * app)
{
  Sound_Effects_PlayerPrivate *priv =
    SOUND_EFFECTS_PLAYER_APPLICATION (app)->priv;
  GtkWidget *cluster_widget;
  GList *cluster_list;
  const gchar *widget_name;
  gchar *cluster_name;

  /* Go through the list of clusters looking for one with the name
   * "cluster_" followed by the cluster number.  */
  for (cluster_list = priv->clusters; cluster_list != NULL;
       cluster_list = cluster_list->next)
    {
      cluster_widget = cluster_list->data;
      widget_name = gtk_widget_get_name (cluster_widget);
      cluster_name = g_strdup_printf ("cluster_%2.2d", cluster_number);
      if (g_ascii_strcasecmp (widget_name, cluster_name) == 0)
        {
          g_free (cluster_name);
          return (cluster_widget);
        }
      g_free (cluster_name);
    }

  return NULL;
}

/* Find the area above the top of the clusters, 
 * so it can be updated.  The parameter passed is the application, which
 * was passed through gstreamer_setup and the gstreamer signaling system
 * as an opaque value.  */
GtkWidget *
sep_get_common_area (GApplication * app)
{
  Sound_Effects_PlayerPrivate *priv =
    SOUND_EFFECTS_PLAYER_APPLICATION (app)->priv;
  GtkWidget *common_area;

  common_area = priv->common_area;

  return (common_area);
}

/* Find the network information.  The parameter passed is the application, which
 * was passed through the various gio callbacks as an opaque value.  */
void *
sep_get_network_data (GApplication * app)
{
  Sound_Effects_PlayerPrivate *priv =
    SOUND_EFFECTS_PLAYER_APPLICATION (app)->priv;
  void *network_data;

  network_data = priv->network_data;
  return (network_data);
}

/* Find the network commands parser information.  
 * The parameter passed is the application.  */
void *
sep_get_parse_net_data (GApplication * app)
{
  Sound_Effects_PlayerPrivate *priv =
    SOUND_EFFECTS_PLAYER_APPLICATION (app)->priv;
  void *parse_net_data;

  parse_net_data = priv->parse_net_data;
  return (parse_net_data);
}

/* Find the top-level window, to use as the transient parent for
 * dialogs. */
GtkWindow *
sep_get_top_window (GApplication * app)
{
  Sound_Effects_PlayerPrivate *priv =
    SOUND_EFFECTS_PLAYER_APPLICATION (app)->priv;
  GtkWindow *top_window;

  top_window = priv->top_window;
  return (top_window);
}

/* Find the project file which contains the parameters. */
xmlDocPtr
sep_get_project_file (GApplication * app)
{
  Sound_Effects_PlayerPrivate *priv =
    SOUND_EFFECTS_PLAYER_APPLICATION (app)->priv;
  xmlDocPtr project_file;

  project_file = priv->project_file;
  return (project_file);
}

/* Remember the project file which contains the parameters. */
void
sep_set_project_file (xmlDocPtr project_file, GApplication * app)
{
  Sound_Effects_PlayerPrivate *priv =
    SOUND_EFFECTS_PLAYER_APPLICATION (app)->priv;

  if (priv->project_file != NULL)
    {
      xmlFreeDoc (priv->project_file);
      priv->project_file = NULL;
    }
  priv->project_file = project_file;

  return;
}

/* Find the name of the project file. */
gchar *
sep_get_project_filename (GApplication * app)
{
  Sound_Effects_PlayerPrivate *priv =
    SOUND_EFFECTS_PLAYER_APPLICATION (app)->priv;
  gchar *project_filename;

  project_filename = priv->project_filename;
  return (project_filename);
}

/* Find the path to the user interface files. */
gchar *
sep_get_ui_path (GApplication * app)
{
  Sound_Effects_PlayerPrivate *priv =
    SOUND_EFFECTS_PLAYER_APPLICATION (app)->priv;
  gchar *ui_path;

  ui_path = priv->ui_path;
  return (ui_path);
}

/* Set the name of the project file. */
void
sep_set_project_filename (gchar * filename, GApplication * app)
{
  Sound_Effects_PlayerPrivate *priv =
    SOUND_EFFECTS_PLAYER_APPLICATION (app)->priv;

  if (priv->project_filename != NULL)
    {
      g_free (priv->project_filename);
      priv->project_filename = NULL;
    }
  priv->project_filename = filename;

  return;
}

/* Find the list of sound effects.  */
GList *
sep_get_sound_list (GApplication * app)
{
  GList *sound_list;

  Sound_Effects_PlayerPrivate *priv =
    SOUND_EFFECTS_PLAYER_APPLICATION (app)->priv;

  sound_list = priv->sound_list;
  return (sound_list);
}

/* Update the list of sound effects.  */
void
sep_set_sound_list (GList * sound_list, GApplication * app)
{
  Sound_Effects_PlayerPrivate *priv =
    SOUND_EFFECTS_PLAYER_APPLICATION (app)->priv;

  priv->sound_list = sound_list;
  return;
}
