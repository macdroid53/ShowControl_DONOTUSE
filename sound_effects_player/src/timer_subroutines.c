/*
 * timer_subroutines.c
 *
 * Copyright © 2016 by John Sauter <John_Sauter@systemeyescomputerstore.com>
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
#include <time.h>
#include <gtk/gtk.h>
#include <gst/gst.h>
#include "timer_subroutines.h"
#include "sound_effects_player.h"

/* When debugging it can be useful to trace what is happening in the
 * timer.  */
#define TRACE_TIMER FALSE

/* the persistent data used by the timer */
struct timer_info
{
  gdouble last_trace_time;
  GList *timer_entry_list;      /* The list of timer items  */
  guint tick_source;
};

/* an entry on the timer list */
struct timer_entry_info
{
  gdouble expiration_time;      /* When to call the subroutine */
  void (*subroutine) (void *, GApplication *);  /* The subroutine to call */
  void *user_data;              /* The first parameter to pass to the 
                                 * subroutine */
};

/* Forward declarations, so I can call these subroutines before I define them.  
 */
static gboolean timer_tick (gpointer user_data);

/* Initialize the timer.  */
void *
timer_init (GApplication * app)
{
  struct timer_info *timer_data;

  /* Allocate the persistent data.  */
  timer_data = g_malloc (sizeof (struct timer_info));

  if (TRACE_TIMER)
    {
      timer_data->last_trace_time = g_get_monotonic_time () / 1e6;
    }

  /* The list of timer entries is empty.  */
  timer_data->timer_entry_list = NULL;

  /* Specify where to go on each tick.  */
  timer_data->tick_source = g_timeout_add (100, timer_tick, app);

  return (timer_data);
}

/* Shut down the timer.  */
void
timer_finalize (GApplication * app)
{
  GList *timer_entry_list;
  struct timer_entry_info *timer_entry_data;
  struct timer_info *timer_data;

  timer_data = sep_get_timer_data (app);
  timer_entry_list = timer_data->timer_entry_list;

  /* Remove all the pending timers.  */
  while (timer_entry_list != NULL)
    {
      GList *timer_entry_next = timer_entry_list->next;
      timer_entry_data = timer_entry_list->data;
      g_free (timer_entry_data);
      timer_data->timer_entry_list =
        g_list_delete_link (timer_data->timer_entry_list, timer_entry_list);
      timer_entry_list = timer_entry_next;
    }

  /* Cancel the ticking source.  */
  g_source_remove (timer_data->tick_source);

  g_free (timer_data);
  timer_data = NULL;
  return;
}

/* Arrange to call back after a specified interval, or slightly later.  */
void
timer_create_entry (void (*subroutine) (void *, GApplication *),
                    gdouble interval, gpointer user_data, GApplication * app)
{
  struct timer_info *timer_data;
  struct timer_entry_info *timer_entry_data;
  gdouble current_time;

  timer_data = sep_get_timer_data (app);
  if (TRACE_TIMER)
    {
      g_print ("create timer entry at %p.\n", subroutine);
    }
  current_time = g_get_monotonic_time () / 1e6;

  /* Construct the timer entry.  */
  timer_entry_data = g_malloc (sizeof (struct timer_entry_info));
  timer_entry_data->subroutine = subroutine;
  timer_entry_data->expiration_time = current_time + interval;
  timer_entry_data->user_data = user_data;

  /* Place it on the timer entry list.  We will see it on the next tick.  */
  timer_data->timer_entry_list =
    g_list_append (timer_data->timer_entry_list, timer_entry_data);
  return;
}

/* Call here every 0.1 seconds to dispatch timers.  */
static gboolean
timer_tick (gpointer user_data)
{
  GApplication *app = user_data;
  gdouble current_time;
  GList *timer_entry_list;
  struct timer_entry_info *timer_entry_data;
  struct timer_info *timer_data;

  /* Get our persistent data.  */
  timer_data = sep_get_timer_data (app);

  /* Calculate the current time in seconds since the last reboot.  */
  current_time = g_get_monotonic_time () / 1e6;

  /* Don't print the trace message oftener than once a second.  */
  if (TRACE_TIMER && ((current_time - timer_data->last_trace_time) >= 1.0))
    {
      g_print ("current time is %f seconds.\n", current_time);
      timer_data->last_trace_time = current_time;
    }

  /* Check the timer entry list for expired timer entries.  For each one found,
   * execute the specified subroutine and remove the entry from the list.  */
  timer_entry_list = timer_data->timer_entry_list;

  while (timer_entry_list != NULL)
    {
      GList *timer_entry_next = timer_entry_list->next;
      timer_entry_data = timer_entry_list->data;
      if (current_time >= timer_entry_data->expiration_time)
        {
          /* The timer has expired.  Call the specified subroutine with
           * its user data and the app as parameters.  */
          if (TRACE_TIMER)
            {
              g_print ("timer routine called at %p.\n",
                       timer_entry_data->subroutine);
            }
          (*timer_entry_data->subroutine) (timer_entry_data->user_data, app);

          /* We are done with this timer entry item.  */
          g_free (timer_entry_data);
          timer_entry_list->data = NULL;
          timer_data->timer_entry_list =
            g_list_delete_link (timer_data->timer_entry_list,
                                timer_entry_list);
        }

      timer_entry_list = timer_entry_next;
    }

  return G_SOURCE_CONTINUE;
}
