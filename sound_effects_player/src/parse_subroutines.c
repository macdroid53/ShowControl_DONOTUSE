/*
 * parse_subroutines.c
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
#include "parse_subroutines.h"
#include "sound_effects_player.h"

/* These subroutines are used to process network messages.
 * Each message is terminated by a new line character ("\n")
 * and consists of a keyword followed by a value.  Upon receiving
 * a command we perform the action specified by the keyword.
 */

/* The persistent data used by the parser.  It is allocated when the
 * parser is initialized, and deallocated when the program terminates.
 * It is accessible from the application.
 */

struct parser_info
{
  GHashTable *hash_table;
  gchar *message_buffer;
};

/* The keyword hash table. */
enum keyword_codes
{ keyword_start = 1, keyword_stop, keyword_quit };

static enum keyword_codes keyword_values[] =
{ keyword_start, keyword_stop, keyword_quit };

struct keyword_value_pairs
{
  gpointer key;
  gpointer val;
};

static struct keyword_value_pairs keywords_with_values[] = {
  {"start", &keyword_values[0]},
  {"stop", &keyword_values[1]},
  {"quit", &keyword_values[2]}
};

/* Initialize the parser */

void *
parse_init (GApplication * app)
{
  struct parser_info *parse_info;
  int i;

  /* Allocate the persistent data used by the parser. */
  parse_info = g_malloc (sizeof (struct parser_info));

  /* Allocate the hash table which holds the keywords. */
  parse_info->hash_table = g_hash_table_new (g_str_hash, g_str_equal);

  /* Populate the hash table. */
  for (i = 0;
       i < sizeof (keywords_with_values) / sizeof (keywords_with_values[0]);
       i++)
    {
      g_hash_table_insert (parse_info->hash_table,
                           keywords_with_values[i].key,
                           keywords_with_values[i].val);
    }

  /* The message buffer starts out empty. */
  parse_info->message_buffer = NULL;

  return parse_info;
}

/* Receive some text from the network.  If we have a complete command,
 * parse and execute it.
 */
void
parse_text (gchar * text, GApplication * app)
{
  struct parser_info *parse_info;
  gchar *new_message;
  gchar *old_message;
  gchar *nl_pointer;
  int command_length;
  gchar *command_string;
  int kl;                       /* keyword length */
  gchar *keyword_string;
  gpointer *p;
  enum keyword_codes keyword_value;
  gchar *extra_text;
  long int cluster_no;

  parse_info = sep_get_parse_info (app);

  /* Append the text we have just received onto any leftover unprocessed text.
   */
  old_message = parse_info->message_buffer;
  if (old_message == NULL)
    {
      new_message = g_strdup (text);
      parse_info->message_buffer = new_message;
    }
  else
    {
      new_message = g_strconcat (old_message, text, NULL);
      parse_info->message_buffer = new_message;
      g_free (old_message);
    }

  /* Process any and all complete commands in the message buffer. */
  while (1)
    {
      new_message = parse_info->message_buffer;
      nl_pointer = g_strstr_len (new_message, -1, "\n");
      if (nl_pointer == NULL)
        /* There are no new line characters in the buffer; wait for
         * more text to arrive. */
        return;

      /* Capture the text up to and including the new line, returning
       * the remainder to the message buffer. */
      command_length = nl_pointer - new_message + 1;
      command_string = g_strndup (new_message, command_length);
      parse_info->message_buffer = g_strdup (nl_pointer + 1);
      g_free (new_message);

      /* Isolate the keyword that starts the command. The keyword will be
       * terminated by white space or the end of the string.  */
      for (kl = 0; kl < command_length; kl++)
        if (g_ascii_isspace (command_string[kl]))
          break;

      keyword_string = g_strndup (command_string, kl);
      /* If there is any text after the keyword, it is probably a parameter
       * to the command.  Isolate it, also. */
      extra_text = g_strdup (command_string + kl);

      /* Find the keyword in the hash table. */
      p = g_hash_table_lookup (parse_info->hash_table, keyword_string);
      if (p == NULL)
        {
          g_print ("Unknown command\n");
        }
      else
        {
          keyword_value = (enum keyword_codes) *p;
          switch (keyword_value)
            {
            case keyword_start:
              cluster_no = strtol (extra_text, NULL, 0);
              sep_start_cluster (cluster_no, app);
              break;
            case keyword_stop:
              cluster_no = strtol (extra_text, NULL, 0);
              sep_stop_cluster (cluster_no, app);
              break;
            case keyword_quit:
              g_application_quit (app);
              break;
            default:
              g_print ("unknown command\n");
            }
        }
      g_free (command_string);
      g_free (keyword_string);
      g_free (extra_text);
    }

  return;
}
