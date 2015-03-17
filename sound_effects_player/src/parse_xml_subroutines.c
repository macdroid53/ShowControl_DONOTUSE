/*
 * parse_xml_subroutines.c
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
#include "parse_xml_subroutines.h"
#include "network_subroutines.h"
#include "sound_effects_player.h"

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
                                                      program_loc->xmlChildrenNode,
                                                      1);
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
void
parse_xml_read_project_file (gchar * project_file_name, GApplication * app)
{
  xmlDocPtr project_file;
  xmlNodePtr current_loc;
  const xmlChar *name;

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
          parse_project_info (project_file, project_file_name, current_loc,
                              app);
          return;
        }
      current_loc = current_loc->next;
    }
  g_printerr ("Not a project file: %s.\n", name);

  return;
}

/* Write the project information to an XML file. */
void
parse_xml_write_project_file (gchar * project_file_name, GApplication * app)
{
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
                              (prop_name, (const xmlChar *) "sound_effects"))
                            {
                              program_loc = equipment_loc->xmlChildrenNode;
                              while (program_loc != NULL)
                                {
                                  name = program_loc->name;
                                  if (xmlStrEqual
                                      (name, (const xmlChar *) "port"))
                                    {
                                      /* This is the "port" node within 
                                       * program sound_effects.  */
                                      port_number = network_get_port (app);
                                      port_number_text =
                                        g_ascii_dtostr (text_buffer,
                                                        G_ASCII_DTOSTR_BUF_SIZE,
                                                        (1.0 * port_number));
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
      xmlSaveFormatFileEnc (project_file_name, project_file, "utf-8", 1);

      /* Remember the file name so we can use it as the default
       * next time. */
      sep_set_project_filename (project_file_name, app);
    }
  else
    {
      g_printerr ("The project file is complex, and must be edited "
                  "with an XML editor such as Emacs.\n");
    }
  return;
}
