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

#include <stdlib.h>
#include <glib.h>
#include <glib-unix.h>
#include <time.h>
#include <sys/time.h>
#include <gtk/gtk.h>
#include <gst/gst.h>
#include "signal_subroutines.h"
#include "sound_effects_player.h"
#include "gstreamer_subroutines.h"

/* When debugging it can be useful to trace what is happening in the
 * signal handler.  */
#define TRACE_SIGNALS FALSE

/* the persistent data used by the signal handler */
struct signal_info
{
  GRecMutex task_mutex;
  GstTask *task;
  GList *timer_list;            /* The list of timer items  */
  sigset_t old_signal_set;
  gboolean task_running;
  gboolean signals_blocked;
};

/* an entry on the timer list */
struct timer_info
{
  gdouble expiration_time;      /* When to call the subroutine */
  void (*subroutine) (void *, GApplication *);  /* The subroutine to call */
  void *user_data;              /* The first parameter to pass to the subroutine */
};

/* Forward declarations, so I can call these subroutines before I define them.  
 */
static void dispatch_timers (gpointer user_data);
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

  /* Create and start the task that will dispatch timers.  */
  g_rec_mutex_init (&signal_data->task_mutex);
  signal_data->timer_list = NULL;
  signal_data->task_running = FALSE;

  signal_data->task = gst_task_new (dispatch_timers, app, NULL);
  gst_task_set_lock (signal_data->task, &signal_data->task_mutex);
  signal_data->task_running = gst_task_start (signal_data->task);

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
  g_print ("signal hup.\n");
  return TRUE;
}

/* Shut down the signal handler.  */
void
signal_finalize (GApplication * app)
{
  GList *timer_item_list;
  struct timer_info *timer_data;
  struct signal_info *signal_data;
  struct sigaction new_action;

  signal_data = sep_get_signal_data (app);
  timer_item_list = signal_data->timer_list;

  /* Remove all the pending timers.  */
  while (timer_item_list != NULL)
    {
      GList *timer_item_next = timer_item_list->next;
      timer_data = timer_item_list->data;
      g_free (timer_data);
      timer_item_list =
        g_list_delete_link (signal_data->timer_list, timer_item_list);
      timer_item_list = timer_item_next;
    }

  /* If the task that is waiting for a signal is running, kill it.  */
  if (signal_data->task_running)
    {
      gst_task_stop (signal_data->task);
      gst_task_join (signal_data->task);
      signal_data->task_running = FALSE;
    }
  g_rec_mutex_clear (&signal_data->task_mutex);

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

/* Task to dispatch timers.  */
static void
dispatch_timers (gpointer user_data)
{
  GApplication *app = user_data;
  sigset_t signal_set;
  siginfo_t info;
  struct timespec signal_timeout;
  GList *timer_item_list;
  struct timer_info *timer_data;
  struct signal_info *signal_data;
  const clockid_t id_m = CLOCK_MONOTONIC;
  struct timespec ts_m;
  gdouble current_time;
  gboolean unexpired_timer_found;
  gdouble soonest_time, wait_time, nanoseconds;
  int wait_result;

  signal_data = sep_get_signal_data (app);

  /* Check the timer list for expired timers.  For each one found,
   * execute the specified subroutine and remove the entry from the list.
   * Also, record the soonest non-expired timer, if any.  */

  clock_gettime (id_m, &ts_m);
  current_time = (double) ts_m.tv_sec + (double) (ts_m.tv_nsec / 1e9);
  unexpired_timer_found = FALSE;
  timer_item_list = signal_data->timer_list;

  while (timer_item_list != NULL)
    {
      GList *timer_item_next = timer_item_list->next;
      timer_data = timer_item_list->data;
      if (current_time >= timer_data->expiration_time)
        {
          /* The timer has expired.  Call the specified subroutine with
           * its user data and the app as parameters.  */
          (*timer_data->subroutine) (timer_data->user_data, app);

          /* We are done with this timer list item.  */
          g_free (timer_data);
          timer_item_list =
            g_list_delete_link (signal_data->timer_list, timer_item_list);
        }
      else
        {
          if (unexpired_timer_found)
            {
              /* We already have at least one unexpired timer.  See if this one
               * expires sooner than any we have seen so far.  */
              if (timer_data->expiration_time < soonest_time)
                {
                  /* It does, remember it in case we don't find one that is
                   * even sooner than this one.  */
                  soonest_time = timer_data->expiration_time;
                }
            }
          else
            {
              /* This is our first unexpired timer.  Remember it in case we
               * don't find a sooner one.  */
              unexpired_timer_found = TRUE;
              soonest_time = timer_data->expiration_time;
            }
        }

      timer_item_list = timer_item_next;
    }

  /* If there are no unexpired timers, limit the wait to 5 seconds anyway.  */
  if (!unexpired_timer_found)
    {
      unexpired_timer_found = TRUE;
      soonest_time = current_time + 5.0;
    }

  /* Wait for a signal or until the next timer expires.  */
  sigemptyset (&signal_set);
  wait_time = soonest_time - current_time;
  if (TRACE_SIGNALS)
    {
      g_print ("wait time is %f seconds.\n", wait_time);
    }
  signal_timeout.tv_sec = (int) wait_time;
  nanoseconds = (wait_time - (double) signal_timeout.tv_sec) * 1e9;
  signal_timeout.tv_nsec = (int) nanoseconds;
  wait_result = sigtimedwait (&signal_set, &info, &signal_timeout);

  if (wait_result < 0)
    {
      if (errno == EINTR)
        {
          /* Interrupted by a signal.  Go back to waiting.  */
          if (TRACE_SIGNALS)
            {
              g_print ("spurious interrupt.\n");
            }
          return;
        }
      if (errno == EAGAIN)
        {
          /* The timer expired.  We can just return because the task will
           * call this subroutine immediately, and it will run all expired
           * timers before waiting.  */
          if (TRACE_SIGNALS)
            {
              g_print ("timer expired.\n");
            }
          return;
        }

      /* Something strange happened.  */
      g_print ("strange error from sigtimedwait: %d.\n", errno);
      return;
    }

  /* We have gotten a success return from sigtimedwait, even though we
   * specified no signals to wait for.  This should not happen.
   */
  g_print ("Success return from sigtimedwait.\n");
  return;
}
