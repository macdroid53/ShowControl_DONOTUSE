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
#include "gstreamer_subroutines.h"
#include "menu_subroutines.h"
#include "network_subroutines.h"
#include "parse_subroutines.h"
#include "button_subroutines.h"

G_DEFINE_TYPE (Sound_Effects_Player, sound_effects_player,
               GTK_TYPE_APPLICATION);

/* ANJUTA: Macro SOUND_EFFECTS_PLAYER_APPLICATION gets Sound_Effects_Player
 *  - DO NOT REMOVE */

/* The private data associated with the top-level window. */
struct _Sound_Effects_PlayerPrivate
{
  /* The Gstreamer pipeline. */
  GstPipeline *pipeline;

  /* The top-level gtk window. */
  GtkWindow *top_window;

  /* The common area, needed for updating the display asynchronously. */
  GtkWidget *common_area;

  /* The list of sounds we can make.  Each item of the GList points
   * to a sound_effect structure. */
  GList *sound_effects;

  /* The list of clusters that might contain sound effects. */
  GList *clusters;

  /* The persistent network information. */
  void *network_data;

  /* The persistent parser information. */
  void *parser_info;

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
  struct sound_effect_str *sound_effect;
  gint cluster_number;
  gchar *cluster_name;
  GtkWidget *cluster_widget;
  GList *cluster_list;
  const gchar *widget_name;
  gchar *sound_name;
  gint i;
  gchar *filename;

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

  if (file != NULL)
    {
      /* TODO: Add code here to open the file in the new window */
    }

  /* Set up the menu. */
  filename = g_strconcat (priv->ui_path, "app-menu.ui", NULL);
  menu_init (app, filename);
  g_free (filename);

  /* Set up Gstreamer for playing tones.  We pass the application so
   * setup_gstreamer can cause it to be passed to the message handler, 
   * which will use it to find the display.  */
  priv->pipeline = gstreamer_init (app);

  /* Set up the remainder of the private data. */
  priv->sound_effects = NULL;

  /* setup_gstreamer created four test tones.  Place them in the first
   * four clusters. */
  for (i = 0; i < 4; i++)
    {
      sound_effect = g_malloc (sizeof (struct sound_effect_str));

      /* Find the cluster that will hold this sound effect. */
      sound_effect->cluster = NULL;
      for (cluster_list = priv->clusters; cluster_list != NULL;
           cluster_list = cluster_list->next)
        {
          cluster_widget = cluster_list->data;
          widget_name = gtk_widget_get_name (cluster_widget);
          cluster_name = g_strdup_printf ("cluster_%2.2d", i);
          if (g_ascii_strcasecmp (widget_name, cluster_name) == 0)
            {
              sound_effect->cluster = cluster_widget;
              break;
            }
          g_free (cluster_name);
        }
      g_free (cluster_name);
      /* Set the name of the sound effect. */
      sound_name = g_strdup_printf ("tone %d", i);
      sound_effect->file_name = sound_name;
      /* Set the pointer to the gstreamer bin that plays the sound. */
      sound_effect->sound_control =
        gstreamer_get_bin (priv->pipeline, sound_name);
      /* Put the cluster number at the top level, so we can search
       * for it quickly. */
      sound_effect->cluster_number = i;
      /* Add this sound effect to the list of sound effects. */
      priv->sound_effects =
        g_list_prepend (priv->sound_effects, sound_effect);
    }

  /* Initialize the message parser. */
  priv->parser_info = parse_init (app);

  /* Listen for network messages. */
  priv->network_data = network_init (app);

  /* The display is initialized; time to show it. */
  gtk_widget_show_all (GTK_WIDGET (top_window));
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
  GstPipeline *pipeline_element;
  GList *sound_effect_list;
  GList *next_sound_effect;
  struct sound_effect_str *sound_effect;
  Sound_Effects_Player *self = (Sound_Effects_Player *) object;

  /* Shut down gstreamer and deallocate all of its storage. */
  pipeline_element = self->priv->pipeline;
  gstreamer_shutdown (pipeline_element);

  /* Deallocate the list of sound effects. */
  sound_effect_list = self->priv->sound_effects;

  while (sound_effect_list != NULL)
    {
      sound_effect = sound_effect_list->data;
      next_sound_effect = sound_effect_list->next;
      g_free (sound_effect->file_name);
      g_free (sound_effect);
      self->priv->sound_effects =
        g_list_delete_link (self->priv->sound_effects, sound_effect_list);
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

/* Find the pipeline, given any widget in the application. */
GstPipeline *
sep_get_pipeline (GtkWidget * object)
{
  GtkWidget *toplevel_widget;
  GtkWindow *toplevel_window;
  GtkApplication *app;
  Sound_Effects_Player *self;
  Sound_Effects_PlayerPrivate *priv;
  GstPipeline *pipeline_element;

  /* Find the top-level window, which has the private data. */
  toplevel_widget = gtk_widget_get_toplevel (object);
  toplevel_window = GTK_WINDOW (toplevel_widget);
  /* Work through the pointer structure to the private data,
   * which includes a pointer to the pipeline. */
  app = gtk_window_get_application (toplevel_window);
  self = SOUND_EFFECTS_PLAYER_APPLICATION (app);
  priv = self->priv;
  pipeline_element = priv->pipeline;
  return (pipeline_element);
}

/* Find the sound effect information corresponding to a cluster, 
 * given a widget inside that cluster. Return NULL if
 * the cluster is not running a sound effect. */
struct sound_effect_str *
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
  struct sound_effect_str *sound_effect = NULL;
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
  sound_effect_list = priv->sound_effects;
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

/* Find the parser information.  The parameter passed is the application.
 */
void *
sep_get_parse_info (GApplication * app)
{
  Sound_Effects_PlayerPrivate *priv =
    SOUND_EFFECTS_PLAYER_APPLICATION (app)->priv;
  void *parser_info;

  parser_info = priv->parser_info;
  return (parser_info);
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

/* Start playing the sound in a specified cluster. */
void
sep_start_cluster (int cluster_no, GApplication * app)
{
  GList *sound_effect_list;
  Sound_Effects_PlayerPrivate *priv =
    SOUND_EFFECTS_PLAYER_APPLICATION (app)->priv;
  GtkWidget *cluster_widget = NULL;
  struct sound_effect_str *sound_effect = NULL;
  gboolean sound_effect_found;
  GList *children_list;
  GtkButton *start_button = NULL;
  const gchar *child_name;

  /* Search through the sound effects for the one attached
   * to this cluster. */
  sound_effect_list = priv->sound_effects;
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
sep_stop_cluster (int cluster_no, GApplication * app)
{
  GList *sound_effect_list;
  Sound_Effects_PlayerPrivate *priv =
    SOUND_EFFECTS_PLAYER_APPLICATION (app)->priv;
  GtkWidget *cluster_widget = NULL;
  struct sound_effect_str *sound_effect = NULL;
  gboolean sound_effect_found;
  GList *children_list;
  GtkButton *stop_button = NULL;
  const gchar *child_name;

  /* Search through the sound effects for the one attached
   * to this cluster. */
  sound_effect_list = priv->sound_effects;
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
