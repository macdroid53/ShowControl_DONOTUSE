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

#include <stdlib.h>
#include <gtk/gtk.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include "menu_subroutines.h"
#include "network_subroutines.h"
#include "sound_effects_player.h"

/* Subroutines used by the menus. */

/* The user has invoked the preferences dialogue from a menu. */
static void
preferences_activated (GSimpleAction * action, GVariant * parameter,
                       gpointer app)
{
  GtkBuilder *builder;
  GError *error = NULL;
  gchar *filename;
  GtkWindow *parent_window;
  GtkDialog *dialog;
  gchar *ui_path;

  /* Load the preferences user interface definition from its file. */
  builder = gtk_builder_new ();
  ui_path = sep_get_ui_path (app);
  filename = g_strconcat (ui_path, "preferences.ui", NULL);
  if (!gtk_builder_add_from_file (builder, filename, &error))
    {
      g_critical ("Couldn't load builder file %s: %s", filename,
                  error->message);
      g_error_free (error);
      return;
    }

  /* Auto-connect signal handlers. */
  gtk_builder_connect_signals (builder, app);

  /* Get the dialog object from the UI file. */
  dialog = GTK_DIALOG (gtk_builder_get_object (builder, "dialog1"));
  if (dialog == NULL)
    {
      g_critical ("Widget \"dialog1\" is missing in file %s.\n", filename);
      return;
    }

  /* We are done with the name of the preferences user interface file
   * and the user interface builder. */
  g_free (filename);
  g_object_unref (G_OBJECT (builder));

  /* Set the dialog window's application, so we can retrieve it later. */
  gtk_window_set_application (GTK_WINDOW (dialog), app);

  /* Get the top-level window to use as the transient parent for
   * the dialog.  This makes sure the dialog appears over the
   * application window.  Also, destroy the dialog if the application
   * is closed, and propagate information such as styling and accessibility
   * from the top-level window to the dialog.  */
  parent_window = sep_get_top_window ((GApplication *) app);
  gtk_window_set_transient_for (GTK_WINDOW (dialog), parent_window);
  gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog), TRUE);
  gtk_window_set_attached_to (GTK_WINDOW (dialog),
                              GTK_WIDGET (parent_window));

  /* Run the dialog and wait for it to complete. */
  gtk_dialog_run (dialog);
  gtk_widget_destroy (GTK_WIDGET (dialog));

  return;
}

/* Subroutine called when the preferences menu changes the network port. */
gboolean
menu_network_port_changed (GtkEntry * port_entry, GtkWidget * dialog_widget)
{
  GApplication *app;
  const gchar *port_text;
  long int port_number;

  /* We are passed the dialogue widget.  We previously set its application
   * so we can retrieve it here. */
  app =
    G_APPLICATION (gtk_window_get_application (GTK_WINDOW (dialog_widget)));

  /* Extract the port number from the entry widget. */
  port_text = gtk_entry_get_text (port_entry);
  port_number = strtoll (port_text, NULL, 10);

  /* Tell the network module to change its port number. */
  network_set_port (port_number, app);

  return TRUE;
}

/* Subroutine called when the preferences dialog is closed. */
gboolean
menu_preferences_close_clicked (GtkButton * close_button,
                                GtkWidget * dialog_widget)
{
  gtk_dialog_response (GTK_DIALOG (dialog_widget), 0);
  return FALSE;
}

/* Subroutine called when the application's "quit" menu item is selected. */
static void
quit_activated (GSimpleAction * action, GVariant * parameter, gpointer app)
{
  g_application_quit (G_APPLICATION (app));
}

/* Reset the project to its defaults. */
static void
new_activated (GSimpleAction * action, GVariant * parameter, gpointer app)
{
  sep_set_project_file (NULL, app);
  network_set_port (1500, app);

  return;
}

/* Dig through the equipment xml file looking for the sound effect player's
 * network port.  When we find it, tell the network module about it. */
static void
parse_equipment_info (xmlDocPtr equipment_file, gchar * equipment_file_name,
                      xmlNodePtr equipment_loc, GApplication * app)
{
  xmlChar *key;
  const xmlChar *name;
  gint64 port_number;
  xmlNodePtr program_loc;
  xmlChar *program_id;

  /* We start at the "equipment" node. */
  /* We are looking for version and program sections. */
  equipment_loc = equipment_loc->xmlChildrenNode;
  while (equipment_loc != NULL)
    {
      name = equipment_loc->name;
      if (xmlStrEqual (name, (const xmlChar *) "version"))
        {
          /* This is the "version" section within "equipment". */
          key =
            xmlNodeListGetString (equipment_file,
                                  equipment_loc->xmlChildrenNode, 1);
          if ((!g_str_has_prefix ((gchar *) key, (gchar *) "1.")))
            {
              g_printerr ("Version number is %s, should start with 1.\n",
                          key);
              return;
            }
          xmlFree (key);
        }
      if (xmlStrEqual (name, (const xmlChar *) "program"))
        {
          /* This is a "program" section.  We only care about the sound
           * effects program. */
          program_id = xmlGetProp (equipment_loc, (const xmlChar *) "id");
          if (xmlStrEqual (program_id, (const xmlChar *) "sound_effects"))
            {
              /* This is the section of the XML file that contains information
               * about the sound effects program.  For now, just extract the
               * network port.  */
              program_loc = equipment_loc->xmlChildrenNode;
              while (program_loc != NULL)
                {
                  name = program_loc->name;
                  if (xmlStrEqual (name, (const xmlChar *) "port"))
                    {
                      /* This is the "port" section within "program".  */
                      key =
                        xmlNodeListGetString (equipment_file,
                                              program_loc->xmlChildrenNode,
                                              1);
                      port_number = g_ascii_strtoll ((gchar *) key, NULL, 10);
                      /* Tell the network module the new port number. */
                      network_set_port (port_number, app);
                      xmlFree (key);
                    }
                  program_loc = program_loc->next;
                }
            }
          xmlFree (program_id);
        }
      equipment_loc = equipment_loc->next;
    }
  return;
}

/* Dig through the project xml file looking for the equipment references.  
 * Parse each one, since the information we are looking for might be scattered 
 * among them.  */
static void
parse_project_info (xmlDocPtr project_file, gchar * project_file_name,
                    xmlNodePtr current_loc, GApplication * app)
{
  xmlChar *key;
  const xmlChar *name;
  xmlChar *prop_name;
  gchar *file_name;
  gchar *file_dirname;
  gchar *absolute_file_name;
  xmlNodePtr equipment_loc;
  xmlNodePtr program_loc;
  xmlDocPtr equipment_file;
  gboolean found_equipment_section;
  const xmlChar *root_name;
  const xmlChar *equipment_name;
  int port_number;

  /* We start at the "project" section.  Important child sections for our 
   * purposes are version and equipment.  */
  current_loc = current_loc->xmlChildrenNode;
  found_equipment_section = FALSE;
  file_name = NULL;
  absolute_file_name = NULL;
  prop_name = NULL;

  while (current_loc != NULL)
    {
      name = current_loc->name;
      if (xmlStrEqual (name, (const xmlChar *) "version"))
        {
          /* This is the "version" section within "project".  We can only
           * interpret version 1 of the project section, so reject all other
           * versions.  The value after the decimal point doesn't matter,
           * since 1.1, for example, will be a compatible extension of 1.0.  */
          key =
            xmlNodeListGetString (project_file, current_loc->xmlChildrenNode,
                                  1);
          if ((!g_str_has_prefix ((gchar *) key, (gchar *) "1.")))
            {
              g_printerr ("Version number is %s, should start with 1.\n",
                          key);
              return;
            }
          xmlFree (key);
        }
      if (xmlStrEqual (name, (const xmlChar *) "equipment"))
        {
          /* This is an "equipment" section within "project". 
             It will have either a reference to the equipment XML file
             or content.  First process the referenced file.  */
          found_equipment_section = TRUE;
          xmlFree (prop_name);
          prop_name = xmlGetProp (current_loc, (const xmlChar *) "href");
          if (prop_name != NULL)
            {
              g_free (file_name);
              file_name = g_strdup ((gchar *) prop_name);
              xmlFree (prop_name);

              /* If the file name specified does not have an absolute path,
               * prepend the path to the project file.  This allows project 
               * files to be copied along with the files they reference.  */
              if (g_path_is_absolute (file_name))
                {
                  absolute_file_name = g_strdup (file_name);
                }
              else
                {
                  file_dirname = g_path_get_dirname (project_file_name);
                  absolute_file_name =
                    g_build_filename (file_dirname, file_name, NULL);
                  g_free (file_dirname);
                }
              g_free (file_name);

              /* Read the specified file as an XML file. */
              xmlLineNumbersDefault (1);
              xmlThrDefIndentTreeOutput (1);
              xmlKeepBlanksDefault (0);
              xmlThrDefTreeIndentString ("    ");
              equipment_file = xmlParseFile (absolute_file_name);
              if (equipment_file == NULL)
                {
                  g_printerr ("Load of equipment file %s failed.\n",
                              absolute_file_name);
                  g_free (absolute_file_name);
                  return;
                }

              /* Make sure the equipment file is valid, then extract data from 
               * it.  */
              equipment_loc = xmlDocGetRootElement (equipment_file);
              if (equipment_loc == NULL)
                {
                  g_printerr ("Empty equipment file: %s.\n",
                              absolute_file_name);
                  xmlFree (equipment_file);
                  g_free (absolute_file_name);
                  return;
                }
              root_name = equipment_loc->name;
              if (xmlStrcmp (root_name, (const xmlChar *) "show_control"))
                {
                  g_printerr ("Not a show_control file: %s; is %s.\n",
                              absolute_file_name, equipment_loc->name);
                  xmlFree (equipment_file);
                  g_free (absolute_file_name);
                  return;
                }

              /* Within the top-level show_control structure should be an 
               * equipment structure.  If there isn't, this isn't an equipment 
               * file and must be rejected. */
              equipment_loc = equipment_loc->xmlChildrenNode;
              equipment_name = NULL;
              while (equipment_loc != NULL)
                {
                  equipment_name = equipment_loc->name;
                  if (xmlStrEqual
                      (equipment_name, (const xmlChar *) "equipment"))
                    {
                      parse_equipment_info (equipment_file,
                                            absolute_file_name, equipment_loc,
                                            app);
                      xmlFree (equipment_file);
                      return;
                    }
                  equipment_loc = equipment_loc->next;
                }
              g_printerr ("Not an equipment file: %s; is %s.\n",
                          absolute_file_name, equipment_name);
              xmlFree (equipment_file);
            }
          /* Now process the content of the equipment section. */
          equipment_loc = current_loc->xmlChildrenNode;
          /* We are looking for the program section with id sound_effects.  */
          while (equipment_loc != NULL)
            {
              name = equipment_loc->name;
              if (xmlStrEqual (name, (const xmlChar *) "program"))
                {
                  prop_name =
                    xmlGetProp (equipment_loc, (const xmlChar *) "id");
                  if (xmlStrEqual
                      (prop_name, (const xmlChar *) "sound_effects"))
                    {
                      program_loc = equipment_loc->xmlChildrenNode;
                      while (program_loc != NULL)
                        {
                          name = program_loc->name;
                          if (xmlStrEqual (name, (const xmlChar *) "port"))
                            {
                              /* This is the "port" section within 
                               * "program".  */
                              key =
                                xmlNodeListGetString (project_file,
                                                      program_loc->
                                                      xmlChildrenNode, 1);
                              port_number =
                                g_ascii_strtoll ((gchar *) key, NULL, 10);
                              /* Tell the network module the new port number. */
                              network_set_port (port_number, app);
                              xmlFree (key);
                            }
                          program_loc = program_loc->next;
                        }
                    }
                }
              equipment_loc = equipment_loc->next;
            }
        }
      current_loc = current_loc->next;
    }
  if (!found_equipment_section)
    {
      g_printerr ("No equipment section in project file: %s.\n",
                  absolute_file_name);
    }
  g_free (absolute_file_name);

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
  gchar *project_file_name;
  xmlDocPtr project_file;
  xmlNodePtr current_loc;
  const xmlChar *name;

  /* Get the top-level window to use as the transient parent for
   * the file open dialog.  This makes sure the dialog appears over the
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
      project_file_name = gtk_file_chooser_get_filename (chooser);
      gtk_widget_destroy (dialog);

      /* Read the file as an XML file. */
      xmlLineNumbersDefault (1);
      xmlThrDefIndentTreeOutput (1);
      xmlKeepBlanksDefault (0);
      xmlThrDefTreeIndentString ("    ");
      project_file = xmlParseFile (project_file_name);
      if (project_file == NULL)
        {
          g_printerr ("Load of project file %s failed.\n", project_file_name);
          g_free (project_file_name);
          return;
        }

      /* Remember the file name. */
      sep_set_project_filename (project_file_name, app);

      /* Remember the data from the XML file, in case we want to refer to it
       * later. */
      sep_set_project_file (project_file, app);

      /* Make sure the project file is valid, then extract data from it. */
      current_loc = xmlDocGetRootElement (project_file);
      if (current_loc == NULL)
        {
          g_printerr ("Empty project file.\n");
          return;
        }
      name = current_loc->name;
      if (!xmlStrEqual (name, (const xmlChar *) "show_control"))
        {
          g_printerr ("Not a show_control file: %s.\n", current_loc->name);
          return;
        }

      /* Within the top-level show_control section should be a project
       * section.  If there isn't, this isn't a project file and must
       * be rejected.  If there is, process it.  */
      current_loc = current_loc->xmlChildrenNode;
      name = NULL;
      while (current_loc != NULL)
        {
          name = current_loc->name;
          if (xmlStrEqual (name, (const xmlChar *) "project"))
            {
              parse_project_info (project_file, project_file_name,
                                  current_loc, app);
              return;
            }
          current_loc = current_loc->next;
        }
      g_printerr ("Not a project file: %s.\n", name);
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
  xmlNodePtr current_loc;
  xmlNodePtr equipment_loc;
  xmlNodePtr project_loc;
  xmlNodePtr program_loc;
  const xmlChar *name;
  xmlChar *prop_name;
  gint port_number;
  gchar *port_number_text;
  gboolean port_number_found;
  gchar text_buffer[G_ASCII_DTOSTR_BUF_SIZE];

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
  gtk_window_set_application (GTK_WINDOW (dialog), app);
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
                         "encoding=\"utf-8\"?> <show_control> <project>"
                         "<version>1.0</version>"
                         "<equipment> <program id=\"sound_effects\">"
                         "<port>1500</port> </program> </equipment>"
                         "</project> </show_control>");
          sep_set_project_file (project_file, app);
        }

      /* The network port might have been changed using the preferences
       * dialogue.  Make sure we write out the current value. */

      /* Find the network node, then set the value of the port node 
       * within it.  This only works if the node number is in the top-level
       * project file.  */
      port_number_found = FALSE;
      current_loc = xmlDocGetRootElement (project_file);
      /* We know the root element is show_control, and there is only one,
       * so we don't have to check it or iterate over it.  */
      current_loc = current_loc->xmlChildrenNode;
      while (current_loc != NULL)
        {
          name = current_loc->name;
          if (xmlStrEqual (name, (const xmlChar *) "project"))
            {
              project_loc = current_loc->xmlChildrenNode;
              while (project_loc != NULL)
                {
                  name = project_loc->name;
                  if (xmlStrEqual (name, (const xmlChar *) "equipment"))
                    {
                      equipment_loc = project_loc->xmlChildrenNode;
                      while (equipment_loc != NULL)
                        {
                          name = equipment_loc->name;
                          if (xmlStrEqual (name, (const xmlChar *) "program"))
                            {
                              prop_name =
                                xmlGetProp (equipment_loc,
                                            (const xmlChar *) "id");
                              if (xmlStrEqual
                                  (prop_name,
                                   (const xmlChar *) "sound_effects"))
                                {
                                  program_loc =
                                    equipment_loc->xmlChildrenNode;
                                  while (program_loc != NULL)
                                    {
                                      name = program_loc->name;
                                      if (xmlStrEqual
                                          (name, (const xmlChar *) "port"))
                                        {
                                          /* This is the "port" node within 
                                           * program sound_effects.  */
                                          port_number =
                                            network_get_port (app);
                                          port_number_text =
                                            g_ascii_dtostr (text_buffer,
                                                            G_ASCII_DTOSTR_BUF_SIZE,
                                                            (1.0 *
                                                             port_number));
                                          xmlNodeSetContent (program_loc,
                                                             (xmlChar *)
                                                             port_number_text);
                                          port_number_found = TRUE;
                                        }
                                      program_loc = program_loc->next;
                                    }
                                }
                            }
                          equipment_loc = equipment_loc->next;
                        }
                    }
                  project_loc = project_loc->next;
                }
            }
          current_loc = current_loc->next;
        }

      if (port_number_found)
        {
          /* Write the file, with indentations to make it easier to edit
           * manually. */
          xmlSaveFormatFileEnc (filename, project_file, "utf-8", 1);

          /* Remember the file name so we can use it as the default
           * next time. */
          sep_set_project_filename (filename, app);
        }
      else
        {
          g_printerr ("The project file is complex, and must be edited "
                      "with an XML editor such as Emacs.\n");
        }
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
