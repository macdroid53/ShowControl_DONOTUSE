/*
 * main.c
 * Copyright © 2015 by John Sauter <John_Sauter@systemeyescomputerstore.com>
 *
 * sound_effects_player is free software: you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * sound_effects_player is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"
#include "sound_effects_player.h"

#include <glib/gi18n.h>

/* The entry point for the sound_effects_player application.  
 * This is a GTK application, so much of what is done here is standard 
 * biolerplate.  See sound_effects_player.c for the beginning of actual 
 * application-specific code.
 */
int
main (int argc, char *argv[])
{
  Sound_Effects_Player *app;
  int status;
  gchar **filenames = NULL;
  const GOptionEntry entries[] = {
    /* you can add your own command line options here */
    {G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY, &filenames,
     "Special option that collects any remaining arguments for us"},
    {NULL,}
  };
  GOptionContext *ctx;
  GError *err = NULL;
  const gchar *nano_str;
  const gchar *check_version_str;
  guint major, minor, micro, nano;

  extern const guint glib_major_version;
  extern const guint glib_minor_version;
  extern const guint glib_micro_version;
  extern const guint glib_binary_age;
  extern const guint glib_interface_age;

#ifdef ENABLE_NLS
  bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);
#endif

  /* Initialize gtk and Gstreamer. */
  gtk_init (&argc, &argv);

  ctx = g_option_context_new ("[FILE1] [FILE2] ...");
  g_option_context_add_group (ctx, gst_init_get_option_group ());
  g_option_context_add_main_entries (ctx, entries, NULL);
  g_option_context_set_summary (ctx, "Create buttons to play the files.");

  if (!g_option_context_parse (ctx, &argc, &argv, &err))
    {
      g_print ("Error initializing: %s\n", GST_STR_NULL (err->message));
      return -1;
    }
  g_option_context_free (ctx);

  /* Print the version of glib that we are linked against. */
  g_print ("This program is linked against glib %d.%d.%d ages %d and %d.\n",
           glib_major_version, glib_minor_version, glib_micro_version,
           glib_binary_age, glib_interface_age);

  /* Print the version of gtk that we are linked against. */
  major = gtk_get_major_version ();
  minor = gtk_get_minor_version ();
  micro = gtk_get_micro_version ();
  g_print ("This program is linked against gtk %d.%d.%d.\n", major, minor,
           micro);

  /* Check that the version of gtk is good. */
  check_version_str =
    gtk_check_version (GTK_MAJOR_VERSION, GTK_MINOR_VERSION,
                       GTK_MICRO_VERSION);
  if (check_version_str != NULL)
    {
      g_print (check_version_str);
      return -1;
    }

  /* Print the version of Gstreamer that we are linked against. */
  gst_version (&major, &minor, &micro, &nano);

  if (nano == 1)
    nano_str = "(CVS)";
  else if (nano == 2)
    nano_str = "(Prerelease)";
  else
    nano_str = "";
  g_print ("This program is linked against GStreamer %d.%d.%d%s.\n", major,
           minor, micro, nano_str);

  /* Initialize gstreamer */
  gst_init (&argc, &argv);

  /* Run the program. */
  app = sound_effects_player_new ();
  status = g_application_run (G_APPLICATION (app), argc, argv);

  /* We are done. */
  g_object_unref (app);

  return status;
}
