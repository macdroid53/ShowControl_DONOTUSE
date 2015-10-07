/*
 * signal_subroutines.c
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

#include <glib.h>
#include <glib-unix.h>
#include <gtk/gtk.h>
#include <gst/gst.h>
#include "signal_subroutines.h"
#include "sound_effects_player.h"
#include "gstreamer_subroutines.h"

/* When debugging it can be useful to trace what is happening in the
 * signal handler.  */
#define TRACE_SIGNALS FALSE

/* the persistent data used by the signal handler */
/* none used at the moment, but we will probably need something
 * to implement sighup.  */

struct signal_info
{
  gboolean not_used;
};

/* Forward declarations, so I can call these subroutines before I define them.  
 */
static gboolean signal_term (gpointer user_data);
static gboolean signal_hup (gpointer user_data);

/* Initialize the signal handler.  */
void *
signal_init (GApplication * app)
{
  struct signal_info *signal_data;

  /* Allocate the persistent data.  */
  signal_data = g_malloc (sizeof (struct signal_info));

  /* Specify the routines to handle signals.  */
  g_unix_signal_add (SIGTERM, signal_term, app);
  g_unix_signal_add (SIGHUP, signal_hup, app);

  return (signal_data);
}

/* Subroutine called when a term signal is received.  */
static gboolean
signal_term (gpointer user_data)
{
  GApplication *app = user_data;

  if (TRACE_SIGNALS)
    {
      g_print ("signal term.\n");
    }

  /* Initiate the shutdown of the gstreamer pipeline.  When it is complete
   * the application will terminate.  */
  gstreamer_shutdown (app);

  return TRUE;
}

/* Subroutine called when a hup signal is received.  */
static gboolean
signal_hup (gpointer user_data)
{
  if (TRACE_SIGNALS)
    {
      g_print ("signal hup.\n");
    }

  /* TODO: shutdown the gstreamer pipeline, re-read the current project,
   * and rebuild the gstreamer pipeline based on it.  */

  return TRUE;
}

/* Shut down the signal handler.  */
void
signal_finalize (GApplication * app)
{
  struct signal_info *signal_data;
  struct sigaction new_action;

  signal_data = sep_get_signal_data (app);

  /* Restore the signals.  */
  new_action.sa_handler = SIG_DFL;
  sigemptyset (&new_action.sa_mask);
  new_action.sa_flags = 0;
  sigaction (SIGTERM, &new_action, NULL);
  sigaction (SIGHUP, &new_action, NULL);

  g_free (signal_data);
  signal_data = NULL;
  return;
}
