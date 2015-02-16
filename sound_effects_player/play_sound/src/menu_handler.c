/*
 * menu_handler.c
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

#include <gtk/gtk.h>
#include <menu_handler.h>
#include <play_sound.h>

/* Subroutines used by the menus. */
static void
preferences_activated (GSimpleAction * action, GVariant * parameter,
                       gpointer app)
{
}

static void
quit_activated (GSimpleAction * action, GVariant * parameter, gpointer app)
{
  g_application_quit (G_APPLICATION (app));
}

static void
new_activated (GSimpleAction * action, GVariant * parameter, gpointer app)
{
}

/* Open a file and read its contents.  The file is assumed to be in GTK
 * key-value format. */
static void
open_activated (GSimpleAction * action, GVariant * parameter, gpointer app)
{
  GtkWidget *dialog;
  GtkFileChooser *chooser;
  GtkFileChooserAction file_action = GTK_FILE_CHOOSER_ACTION_OPEN;
  GtkWindow *parent_window;
  gint res;
  GKeyFileFlags flags;
  char *filename;
  GKeyFile *parameter_file;
  GError *err = NULL;

  /* Get the top-level window, to use as the transient parent for
   * the dialog.  This makes sure the dialog appears over the
   * application window. */
  parent_window = play_sound_get_top_window ((GApplication *) app);

  dialog =
    gtk_file_chooser_dialog_new ("Open File", parent_window, file_action,
                                 "_Cancel", GTK_RESPONSE_CANCEL, "_Open",
                                 GTK_RESPONSE_ACCEPT, NULL);
  /* Use the file dialog to ask for a file to read. */
  res = gtk_dialog_run (GTK_DIALOG (dialog));
  if (res == GTK_RESPONSE_ACCEPT)
    {
      /* We have a file name. */
      chooser = GTK_FILE_CHOOSER (dialog);
      filename = gtk_file_chooser_get_filename (chooser);
      g_print ("%s\n", filename);
      /* Read the file as a key value file, with [] to name groups, and
       * keyword=value within groups. */
      parameter_file = g_key_file_new ();
      /* Keep the comments and translations, since after we change a
       * value we will want to write this file back out. */
      flags = (G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS);
      g_key_file_load_from_file (parameter_file, filename, flags, &err);
      if (err != NULL)
        {
          g_printerr ("Load from file %s failed: %s\n", filename,
                      err->message);
          g_error_free (err);
          g_free (filename);
          gtk_widget_destroy (dialog);
          return;
        }

      g_free (filename);
      /* Remember the key value file, in case we want to write it out later. */
      play_sound_set_parameter_file (parameter_file, app);
      
      /* TODO: Do something with the information read. */
    }
  gtk_widget_destroy (dialog);

  return;
}

static void
save_activated (GSimpleAction * action, GVariant * parameter, gpointer app)
{
}

static void
copy_activated (GSimpleAction * action, GVariant * parameter, gpointer app)
{
}

static void
cut_activated (GSimpleAction * action, GVariant * parameter, gpointer app)
{
}

static void
paste_activated (GSimpleAction * action, GVariant * parameter, gpointer app)
{
}


/* Actions table to link menu items to subroutines. */
static GActionEntry app_entries[] = {
  {"preferences", preferences_activated, NULL, NULL, NULL},
  {"quit", quit_activated, NULL, NULL, NULL},
  {"new", new_activated, NULL, NULL, NULL},
  {"open", open_activated, NULL, NULL, NULL},
  {"save", save_activated, NULL, NULL, NULL},
  {"copy", copy_activated, NULL, NULL, NULL},
  {"cut", cut_activated, NULL, NULL, NULL},
  {"paste", paste_activated, NULL, NULL, NULL}
};

/* Initialize the menu. */
void
play_sound_menu_init (GApplication * app, gchar * file_name)
{
  GtkBuilder *builder;
  GError *error = NULL;
  GMenuModel *app_menu;
  GMenuModel *menu_bar;

  const gchar *quit_accels[2] = { "<Ctrl>Q", NULL };

  g_action_map_add_action_entries (G_ACTION_MAP (app), app_entries,
                                   G_N_ELEMENTS (app_entries), app);
  gtk_application_set_accels_for_action (GTK_APPLICATION (app), "app.quit",
                                         quit_accels);

  /* Load UI from file */
  builder = gtk_builder_new ();
  if (!gtk_builder_add_from_file (builder, file_name, &error))
    {
      g_critical ("Couldn't load menu file %s: %s", file_name,
                  error->message);
      g_error_free (error);
    }

  /* Auto-connect signal handlers. */
  gtk_builder_connect_signals (builder, app);

  /* Specify the application menu and the menu bar. */
  app_menu = (GMenuModel *) gtk_builder_get_object (builder, "appmenu");
  menu_bar = (GMenuModel *) gtk_builder_get_object (builder, "menubar");
  gtk_application_set_app_menu (GTK_APPLICATION (app), app_menu);
  gtk_application_set_menubar (GTK_APPLICATION (app), menu_bar);

  g_object_unref (builder);

  return;
}
