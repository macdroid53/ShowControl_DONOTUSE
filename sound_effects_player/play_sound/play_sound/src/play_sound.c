/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 2; tab-width: 2 -*-  */
/*
 * play_sound.c
 * Copyright © 2015 John Sauter <John_Sauter@systemeyescomputerstore.com>
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
#include "gstreamer_utils.h"
#include <glib/gi18n.h>

/* For testing purposes use the local (not installed) ui file */
#define UI_FILE PACKAGE_DATA_DIR"/ui/play_sound.ui"
/* #define UI_FILE "src/play_sound.ui" */

/* We search for this window in the UI file to find the top-level
 * application window.
 */
#define TOP_WINDOW "window"

G_DEFINE_TYPE (Play_Sound, play_sound, GTK_TYPE_APPLICATION);

/* ANJUTA: Macro PLAY_SOUND_APPLICATION gets Play_Sound - DO NOT REMOVE */

/* The private data associated with the top-level window. */
struct _Play_SoundPrivate
{
  /* The top-level Gstreamer pipeline. */
  GstElement *pipeline;
  /* The list of sounds we can make.  Each item of the GList points
   * to a sound_effect structure. */
  GList *sound_effects;

  /* ANJUTA: Widgets declaration for play_sound.ui - DO NOT REMOVE */
};

/* Create a new window loading a file. */
static void
play_sound_new_window (GApplication * app, GFile * file)
{
  GtkWidget *window;

  GtkBuilder *builder;
  GError *error = NULL;

  Play_SoundPrivate *priv = PLAY_SOUND_APPLICATION (app)->priv;

  /* Load UI from file */
  builder = gtk_builder_new ();
  if (!gtk_builder_add_from_file (builder, UI_FILE, &error))
    {
      g_critical ("Couldn't load builder file: %s", error->message);
      g_error_free (error);
    }

  /* Auto-connect signal handlers */
  gtk_builder_connect_signals (builder, app);

  /* Get the window object from the ui file */
  window = GTK_WIDGET (gtk_builder_get_object (builder, TOP_WINDOW));
  if (!window)
    {
      g_critical ("Widget \"%s\" is missing in file %s.", TOP_WINDOW,
		  UI_FILE);
    }

  /* ANJUTA: Widgets initialization for play_sound.ui - DO NOT REMOVE */

  g_object_unref (builder);

  gtk_window_set_application (GTK_WINDOW (window), GTK_APPLICATION (app));

  if (file != NULL)
    {
      /* TODO: Add code here to open the file in the new window */
    }
  /* Set up Gstreamer for playing tones. */
  priv->pipeline = setup_gstreamer ();

  gtk_widget_show_all (GTK_WIDGET (window));
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
  GstElement *pipeline_element;

  Play_Sound *self = (Play_Sound *) object;

  pipeline_element = self->priv->pipeline;
  shutdown_gstreamer (pipeline_element);
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

/* Find a cluster's pipeline, given any widget in the cluster. */
GstElement *
play_sound_get_pipeline (GtkWidget * object)
{
  GtkWidget *toplevel_widget;
  GtkWindow *toplevel_window;
  GtkApplication *app;
  Play_Sound *self;
  Play_SoundPrivate *priv;
  GstElement *pipeline_element;

	/* Find the top-level window, which has the private data. */
  toplevel_widget = gtk_widget_get_toplevel (object);
  toplevel_window = GTK_WINDOW (toplevel_widget);
	/* Work through the pointer structure to the private data,
	 * which includes the pipeline. */
  app = gtk_window_get_application (toplevel_window);
  self = PLAY_SOUND_APPLICATION (app);
  priv = self->priv;
  pipeline_element = priv->pipeline;
  return (pipeline_element);
}

/* Find the sound effect information corresponding to a cluster, 
 * given any widget in the cluster. */
struct sound_effect *
play_sound_get_sound_effect (GtkWidget * object)
{
  /* TODO */
  return NULL;
}
