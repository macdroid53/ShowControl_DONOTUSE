/*
 * menu_subroutines.c
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
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include "menu_subroutines.h"
#include "network_subroutines.h"
#include "sound_effects_player.h"

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

/* Reset the project to its defaults. */
static void
new_activated (GSimpleAction * action, GVariant * parameter, gpointer app)
{
  xmlDocPtr project_file;

  xmlLineNumbersDefault (1);
  xmlThrDefIndentTreeOutput (1);
  xmlKeepBlanksDefault (0);
  xmlThrDefTreeIndentString ("    ");
  project_file =
    xmlParseDoc ((xmlChar *) "<?xml version=\"1.0\" "
                 "encoding=\"utf-8\"?> <project>"
                 "<general> <version>1.0</version>"
                 "</general> <network> <port>1500</port>"
                 "</network> </project>");
  sep_set_project_file (project_file, app);
  network_set_port (1500, app);

  return;
}

/* Dig through the xml file looking for the network port.  When we find it,
 * tell the network module about it. */
static void
parse_network_info (xmlDocPtr project_file, xmlNodePtr current_loc,
                    GApplication * app)
{
  xmlChar *key;
  const xmlChar *name;
  gint port_number;

  current_loc = current_loc->xmlChildrenNode;
  while (current_loc != NULL)
    {
      name = current_loc->name;
      if ((!xmlStrcmp (name, (const xmlChar *) "port")))
        {
          key =
            xmlNodeListGetString (project_file, current_loc->xmlChildrenNode,
                                  1);
          port_number = atol ((char *) key);
          network_set_port (port_number, app);
          xmlFree (key);
        }
      current_loc = current_loc->next;
    }
  return;
}

/* Open a project file and read its contents.  The file is assumed to be in 
 * XML format. */
static void
open_activated (GSimpleAction * action, GVariant * parameter, gpointer app)
{
  GtkWidget *dialog;
  GtkFileChooser *chooser;
  GtkFileChooserAction file_action = GTK_FILE_CHOOSER_ACTION_OPEN;
  GtkWindow *parent_window;
  gint res;
  gchar *filename;
  xmlDocPtr project_file;
  xmlNodePtr current_loc;
  const xmlChar *name;

  /* Get the top-level window to use as the transient parent for
   * the dialog.  This makes sure the dialog appears over the
   * application window. */
  parent_window = sep_get_top_window ((GApplication *) app);

  /* Configure the dialogue: choosing multiple files is not permitted. */
  dialog =
    gtk_file_chooser_dialog_new ("Open Project File", parent_window,
                                 file_action, "_Cancel", GTK_RESPONSE_CANCEL,
                                 "_Open", GTK_RESPONSE_ACCEPT, NULL);
  chooser = GTK_FILE_CHOOSER (dialog);
  gtk_file_chooser_set_select_multiple (chooser, FALSE);

  /* Use the file dialog to ask for a file to read. */
  res = gtk_dialog_run (GTK_DIALOG (dialog));
  if (res == GTK_RESPONSE_ACCEPT)
    {
      /* We have a file name. */
      filename = gtk_file_chooser_get_filename (chooser);
      gtk_widget_destroy (dialog);

      /* Read the file as an XML file. */
      xmlLineNumbersDefault (1);
      xmlThrDefIndentTreeOutput (1);
      xmlKeepBlanksDefault (0);
      xmlThrDefTreeIndentString ("    ");
      project_file = xmlParseFile (filename);
      if (project_file == NULL)
        {
          g_printerr ("Load from project file %s failed.\n", filename);
          g_free (filename);
          return;
        }

      /* Remember the file name. */
      sep_set_project_filename (filename, app);

      /* Remember the data from the XML file, in case we want to write it out 
       * later. */
      sep_set_project_file (project_file, app);

      /* The only item currently in the project file is the
       * network port.  Fetch it. */
      current_loc = xmlDocGetRootElement (project_file);
      if (current_loc == NULL)
        {
          g_printerr ("Empty project file.\n");
          return;
        }
      name = current_loc->name;
      if (xmlStrcmp (name, (const xmlChar *) "project"))
        {
          g_printerr ("Not a project file: %s.\n", current_loc->name);
          return;
        }
      current_loc = current_loc->xmlChildrenNode;
      while (current_loc != NULL)
        {
          name = current_loc->name;
          if ((!xmlStrcmp (name, (const xmlChar *) "network")))
            {
              parse_network_info (project_file, current_loc, app);
            }
          current_loc = current_loc->next;
        }
    }
  return;
}

static void
save_activated (GSimpleAction * action, GVariant * parameter, gpointer app)
{
}

/* Write the project information to an XML file. */
static void
save_as_activated (GSimpleAction * action, GVariant * parameter, gpointer app)
{

  GtkWidget *dialog;
  GtkFileChooser *chooser;
  GtkFileChooserAction file_action = GTK_FILE_CHOOSER_ACTION_SAVE;
  GtkWindow *parent_window;
  gint res;
  gchar *filename;
  xmlDocPtr project_file;

  /* Get the top-level window to use as the transient parent for
   * the dialog.  This makes sure the dialog appears over the
   * application window. */
  parent_window = sep_get_top_window ((GApplication *) app);

  /* Configure the dialogue: prompt on specifying an existing file,
   * allow folder creation, and specify the current project file
   * name if one exists, or a default name if not. */
  dialog =
    gtk_file_chooser_dialog_new ("Save Project File", parent_window,
                                 file_action, "_Cancel", GTK_RESPONSE_CANCEL,
                                 "_Save", GTK_RESPONSE_ACCEPT, NULL);
  chooser = GTK_FILE_CHOOSER (dialog);
  gtk_file_chooser_set_do_overwrite_confirmation (chooser, TRUE);
  gtk_file_chooser_set_create_folders (chooser, TRUE);
  filename = sep_get_project_filename (app);
  if (filename == NULL)
    {
      filename = g_strdup ("Nameless_project.xml");
      sep_set_project_filename (filename, app);
    }
  gtk_file_chooser_set_filename (chooser, filename);

  /* Use the file dialog to ask for a file to write. */
  res = gtk_dialog_run (GTK_DIALOG (dialog));
  if (res == GTK_RESPONSE_ACCEPT)
    {
      /* We have a file name. */
      filename = gtk_file_chooser_get_filename (chooser);
      gtk_widget_destroy (dialog);

      /* Write the project data as an XML file. */
      project_file = sep_get_project_file (app);
      if (project_file == NULL)
        {
          /* We don't have a project file--create one. */
          xmlLineNumbersDefault (1);
          xmlThrDefIndentTreeOutput (1);
          xmlKeepBlanksDefault (0);
          xmlThrDefTreeIndentString ("    ");
          project_file =
            xmlParseDoc ((xmlChar *) "<?xml version=\"1.0\" "
                         "encoding=\"utf-8\"?> <project>"
                         "<general> <version>1.0</version>"
                         "</general> <network> <port>1500</port>"
                         "</network> </project>");
          sep_set_project_file (project_file, app);
        }
      xmlSaveFormatFileEnc (filename, project_file, "utf-8", 1);
      sep_set_project_filename (filename, app);
    }

  return;
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
  {"save_as", save_as_activated, NULL, NULL, NULL},
  {"copy", copy_activated, NULL, NULL, NULL},
  {"cut", cut_activated, NULL, NULL, NULL},
  {"paste", paste_activated, NULL, NULL, NULL}
};

/* Initialize the menu. */
void
menu_init (GApplication * app, gchar * file_name)
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
