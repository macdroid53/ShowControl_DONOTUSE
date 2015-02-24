/*
 * play_sound.c
 * Copyright © 2015 by John Sauter <John_Sauter@systemeyescomputerstore.com>
 * 
 * play_sound is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * play_sound is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "play_sound.h"
#include "gstreamer_subroutines.h"
#include "menu_subroutines.h"
#include "network_subroutines.h"
#include "parse_subroutines.h"
#include "button_subroutines.h"
#include <glib/gi18n.h>

/* For testing purposes use the local (not installed) ui file */
#define UI_FILE PACKAGE_DATA_DIR"/ui/play_sound.ui"
/* #define UI_FILE "src/play_sound.ui" */

/* We search for this ID in the UI file to find the top-level
 * application window.
 */
#define TOP_WINDOW "top_level_window"

G_DEFINE_TYPE (Play_Sound, play_sound, GTK_TYPE_APPLICATION);

/* ANJUTA: Macro PLAY_SOUND_APPLICATION gets Play_Sound - DO NOT REMOVE */

/* The private data associated with the top-level window. */
struct _Play_SoundPrivate
{
  /* The top-level Gstreamer pipeline. */
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

  /* The key-value file that holds parameters for the program. */
  GKeyFile *parameter_file;

  /* ANJUTA: Widgets declaration for play_sound.ui - DO NOT REMOVE */
};

/* Create a new window loading a file. */
static void
play_sound_new_window (GApplication * app, GFile * file)
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

  Play_SoundPrivate *priv = PLAY_SOUND_APPLICATION (app)->priv;

  /* Load UI from file */
  builder = gtk_builder_new ();
  if (!gtk_builder_add_from_file (builder, UI_FILE, &error))
    {
      g_critical ("Couldn't load builder file: %s", error->message);
      g_error_free (error);
    }

  /* Auto-connect signal handlers. */
  gtk_builder_connect_signals (builder, app);

  /* Get the top-level window object from the ui file. */
  top_window = GTK_WINDOW (gtk_builder_get_object (builder, TOP_WINDOW));
  priv->top_window = top_window;
  if (!top_window)
    {
      g_critical ("Widget \"%s\" is missing in file %s.", TOP_WINDOW,
                  UI_FILE);
    }

  /* Also get the common area. */
  common_area = GTK_WIDGET (gtk_builder_get_object (builder, "common_area"));
  priv->common_area = GTK_WIDGET (common_area);
  if (!common_area)
    {
      g_critical ("Widget common_area is missing in file %s.", UI_FILE);
    }

  /* Remember where the clusters are. */
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

  /* ANJUTA: Widgets initialization for play_sound.ui - DO NOT REMOVE */

  g_object_unref (builder);

  gtk_window_set_application (top_window, GTK_APPLICATION (app));

  if (file != NULL)
    {
      /* TODO: Add code here to open the file in the new window */
    }

  /* Set up the menu. */
  menu_init (app, PACKAGE_DATA_DIR "/ui/app-menu.ui");

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
        gstreamer_find_bin (priv->pipeline, sound_name);
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
play_sound_activate (GApplication * application)
{
  play_sound_new_window (application, NULL);
}

static void
play_sound_open (GApplication * application, GFile ** files, gint n_files,
                 const gchar * hint)
{
  gint i;

  for (i = 0; i < n_files; i++)
    play_sound_new_window (application, files[i]);
}

static void
play_sound_init (Play_Sound * object)
{
  object->priv =
    G_TYPE_INSTANCE_GET_PRIVATE (object, PLAY_SOUND_TYPE_APPLICATION,
                                 Play_SoundPrivate);
}

static void
play_sound_finalize (GObject * object)
{
  GstPipeline *pipeline_element;
  GList *sound_effect_list;
  GList *next_sound_effect;
  struct sound_effect_str *sound_effect;
  Play_Sound *self = (Play_Sound *) object;

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

  G_OBJECT_CLASS (play_sound_parent_class)->finalize (object);
}

static void
play_sound_class_init (Play_SoundClass * klass)
{
  G_APPLICATION_CLASS (klass)->activate = play_sound_activate;
  G_APPLICATION_CLASS (klass)->open = play_sound_open;

  g_type_class_add_private (klass, sizeof (Play_SoundPrivate));

  G_OBJECT_CLASS (klass)->finalize = play_sound_finalize;
}

Play_Sound *
play_sound_new (void)
{
  return g_object_new (play_sound_get_type (), "application-id",
                       "org.gnome.play_sound", "flags",
                       G_APPLICATION_HANDLES_OPEN, NULL);
}

/* Find the pipeline, given any widget in the application. */
GstPipeline *
play_sound_get_pipeline (GtkWidget * object)
{
  GtkWidget *toplevel_widget;
  GtkWindow *toplevel_window;
  GtkApplication *app;
  Play_Sound *self;
  Play_SoundPrivate *priv;
  GstPipeline *pipeline_element;

  /* Find the top-level window, which has the private data. */
  toplevel_widget = gtk_widget_get_toplevel (object);
  toplevel_window = GTK_WINDOW (toplevel_widget);
  /* Work through the pointer structure to the private data,
   * which includes a pointer to the pipeline. */
  app = gtk_window_get_application (toplevel_window);
  self = PLAY_SOUND_APPLICATION (app);
  priv = self->priv;
  pipeline_element = priv->pipeline;
  return (pipeline_element);
}

/* Find the sound effect information corresponding to a cluster, 
 * given a widget inside that cluster. Return NULL if
 * the cluster is not running a sound effect. */
struct sound_effect_str *
play_sound_get_sound_effect (GtkWidget * object)
{
  GtkWidget *this_object;
  const gchar *widget_name;
  GtkWidget *cluster_widget = NULL;
  GtkWidget *toplevel_widget;
  GtkWindow *toplevel_window;
  GtkApplication *app;
  Play_Sound *self;
  Play_SoundPrivate *priv;
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
  self = PLAY_SOUND_APPLICATION (app);
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
play_sound_find_common_area (GApplication * app)
{
  Play_SoundPrivate *priv = PLAY_SOUND_APPLICATION (app)->priv;
  GtkWidget *common_area;

  common_area = priv->common_area;

  return (common_area);
}

/* Find the network information.  The parameter passed is the application, which
 * was passed through the various gio callbacks as an opaque value.  */
void *
play_sound_get_network_data (GApplication * app)
{
  Play_SoundPrivate *priv = PLAY_SOUND_APPLICATION (app)->priv;
  void *network_data;

  network_data = priv->network_data;
  return (network_data);
}

/* Find the parser information.  The parameter passed is the application.
 */
void *
play_sound_get_parse_info (GApplication * app)
{
  Play_SoundPrivate *priv = PLAY_SOUND_APPLICATION (app)->priv;
  void *parser_info;

  parser_info = priv->parser_info;
  return (parser_info);
}

/* Find the top-level window, to use as the transient parent for
 * dialogs. */
GtkWindow *
play_sound_get_top_window (GApplication * app)
{
  Play_SoundPrivate *priv = PLAY_SOUND_APPLICATION (app)->priv;
  GtkWindow *top_window;

  top_window = priv->top_window;
  return (top_window);
}

/* Find the key-value file which contains the parameters. */
GKeyFile *
play_sound_get_parameter_file (GApplication * app)
{
  Play_SoundPrivate *priv = PLAY_SOUND_APPLICATION (app)->priv;
  GKeyFile *parameter_file;

  parameter_file = priv->parameter_file;
  return (parameter_file);
}

/* Set the key-value file which contains the parameters. */
void
play_sound_set_parameter_file (GKeyFile * parameter_file, GApplication * app)
{
  Play_SoundPrivate *priv = PLAY_SOUND_APPLICATION (app)->priv;

  if (priv->parameter_file != NULL)
    {
      g_key_file_free (priv->parameter_file);
      priv->parameter_file = NULL;
    }
  priv->parameter_file = parameter_file;
  return;
}

/* Start playing the sound in a specified cluster. */
void
play_sound_start_cluster (int cluster_no, GApplication * app)
{
  GList *sound_effect_list;
  Play_SoundPrivate *priv = PLAY_SOUND_APPLICATION (app)->priv;
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
play_sound_stop_cluster (int cluster_no, GApplication * app)
{
  GList *sound_effect_list;
  Play_SoundPrivate *priv = PLAY_SOUND_APPLICATION (app)->priv;
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
