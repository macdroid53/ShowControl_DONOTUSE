/*
 * message_subroutines.c
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

/* GValueArray has been depreciated in favor of Garray since GLib 2.32,
 * but the "good" plugin named level still uses it in Gstreamer 1.4.  
 * Maybe I will rewrite the level plugin to use Garray, but until then 
 * disable GLib depreciation warnings. */
#define GLIB_DISABLE_DEPRECATION_WARNINGS

#include "message_subroutines.h"
#include <math.h>
#include <gst/gst.h>
#include "display_subroutines.h"
#include "sound_subroutines.h"
#include "gstreamer_subroutines.h"

/* When debugging, it is sometimes useful to have printouts of the
 * messages as they happen. */
#define TRACE_MESSAGES FALSE

/* Process a message from the pipeline. User_data is the 
 * application, so we can reach the display.  */
gboolean
message_handler (GstBus * bus_element, GstMessage * message,
                 gpointer user_data)
{

  switch (GST_MESSAGE_TYPE (message))
    {
    case GST_MESSAGE_ELEMENT:
      /* This is a special-purpose message from an element.  */
      {
        const GstStructure *s = gst_message_get_structure (message);

        if (gst_structure_has_name (s, (gchar *) "level"))
          {
            /* The level message shows the sound level on each channel.  */
            gint channels;
            GstClockTime endtime;
            gdouble rms_dB, peak_dB, decay_dB;
            gdouble rms;
            const GValue *array_val;
            const GValue *value;
            GValueArray *rms_arr, *peak_arr, *decay_arr;
            gint i;

            if (!gst_structure_get_clock_time (s, "endtime", &endtime))
              g_warning ("Could not parse endtime.");

            /* The values are packed into GValueArrays 
             * with the value per channel. */
            array_val = gst_structure_get_value (s, "rms");
            rms_arr = (GValueArray *) g_value_get_boxed (array_val);

            array_val = gst_structure_get_value (s, "peak");
            peak_arr = (GValueArray *) g_value_get_boxed (array_val);

            array_val = gst_structure_get_value (s, "decay");
            decay_arr = (GValueArray *) g_value_get_boxed (array_val);

            /* We can get the number of channels as the length of any of the 
             * value arrays. */
            channels = rms_arr->n_values;

            for (i = 0; i < channels; ++i)
              {
                value = g_value_array_get_nth (rms_arr, i);
                rms_dB = g_value_get_double (value);

                value = g_value_array_get_nth (peak_arr, i);
                peak_dB = g_value_get_double (value);

                value = g_value_array_get_nth (decay_arr, i);
                decay_dB = g_value_get_double (value);

                /* Converting from dB to normal gives us a value between 
                 * 0.0 and 1.0. */
                rms = pow (10, rms_dB / 20);
                display_update_vu_meter (user_data, i, rms, peak_dB,
                                         decay_dB);
              }
            break;
          }

        /* Check for a forwarded message from a bin.  */
        if (gst_structure_has_name (s, (gchar *) "GstBinForwarded"))
          {
            GstMessage *forward_msg = NULL;

            gst_structure_get (s, "message", GST_TYPE_MESSAGE, &forward_msg,
                               NULL);
            if (GST_MESSAGE_TYPE (forward_msg) == GST_MESSAGE_EOS)
              {
                if (TRACE_MESSAGES)
                  {
                    g_print ("Forwarded EOS from element %s.\n",
                             GST_OBJECT_NAME (GST_MESSAGE_SRC (forward_msg)));
                  }
                break;
              }
          }

        if (gst_structure_has_name (s, (gchar *) "completed"))
          {
            /* The completed message means a sound has entered the release
             * portion of its envelope.  */
            const gchar *sound_name;

            /* The structure in the message contains the name of the sound.  
             */
            sound_name = gst_structure_get_string (s, (gchar *) "sound_name");
            sound_completed (sound_name, G_APPLICATION (user_data));
          }

        if (gst_structure_has_name (s, (gchar *) "terminated"))
          {
            /* The terminated message means a sound has entered the release
             * portion of its envelope due to an external event.  */
            const gchar *sound_name;

            /* The structure in the message contains the name of the sound.  
             */
            sound_name = gst_structure_get_string (s, (gchar *) "sound_name");
            sound_terminated (sound_name, G_APPLICATION (user_data));
          }

        /* Catchall for unrecognized messages */
        if (TRACE_MESSAGES)
          {
            g_print (" Message element: %s from %s.\n",
                     gst_structure_get_name (s),
                     GST_OBJECT_NAME (message->src));
          }
        break;
      }

    case GST_MESSAGE_EOS:
      {
        if (TRACE_MESSAGES)
          {
            g_print ("EOS from %s.\n", GST_OBJECT_NAME (message->src));
          }

        gstreamer_process_eos (user_data);

        break;
      }

    case GST_MESSAGE_ERROR:
      {
        gchar *debug = NULL;
        GError *err = NULL;

        gst_message_parse_error (message, &err, &debug);
        g_print ("Error: %s.\n", err->message);
        g_error_free (err);

        if (debug)
          {
            g_print ("  Debug details: %s.\n", debug);
            g_free (debug);
          }
        g_application_quit (user_data);
        break;
      }

    case GST_MESSAGE_STATE_CHANGED:
      {
        GstState old_state, new_state, pending_state;

        gst_message_parse_state_changed (message, &old_state, &new_state,
                                         &pending_state);
        if (TRACE_MESSAGES)
          {
            g_print ("Element %s has changed state from %s to %s, "
                     "pending %s.\n", GST_OBJECT_NAME (message->src),
                     gst_element_state_get_name (old_state),
                     gst_element_state_get_name (new_state),
                     gst_element_state_get_name (pending_state));
          }
        break;
      }

    case GST_MESSAGE_RESET_TIME:
      {
        guint64 running_time;
        const gchar *source;

        gst_message_parse_reset_time (message, &running_time);
        source = GST_OBJECT_NAME (message->src);
        if (TRACE_MESSAGES)
          {
            g_print ("Reset time to %ld by %s.\n", running_time, source);
          }
        break;
      }

    case GST_MESSAGE_STREAM_STATUS:
      {
        GstStreamStatusType status_type;
        GstElement *owner;
        gchar *status_text;

        gst_message_parse_stream_status (message, &status_type, &owner);
        switch (status_type)
          {
          case GST_STREAM_STATUS_TYPE_CREATE:
            status_text = (gchar *) "create";
            break;
          case GST_STREAM_STATUS_TYPE_ENTER:
            status_text = (gchar *) "enter";
            break;
          case GST_STREAM_STATUS_TYPE_LEAVE:
            status_text = (gchar *) "leave";
            break;
          case GST_STREAM_STATUS_TYPE_DESTROY:
            status_text = (gchar *) "destroy";
            break;
          case GST_STREAM_STATUS_TYPE_START:
            status_text = (gchar *) "start";
            break;
          case GST_STREAM_STATUS_TYPE_PAUSE:
            status_text = (gchar *) "pause";
            break;
          case GST_STREAM_STATUS_TYPE_STOP:
            status_text = (gchar *) "stop";
            break;
          default:
            status_text = (gchar *) "unknown";
            break;
          }
        if (TRACE_MESSAGES)
          {
            g_print ("Stream status of %s from %s.\n", status_text,
                     GST_OBJECT_NAME (owner));
          }
        break;
      }

    case GST_MESSAGE_ASYNC_DONE:
      {
        if (TRACE_MESSAGES)
          {
            g_print ("Async-done from %s.\n", GST_OBJECT_NAME (message->src));
          }

        /* The pipeline has completed an asynchronous operation.  */
        gstreamer_async_done (user_data);
        break;
      }

    default:
      {
        if (TRACE_MESSAGES)
          {
            g_print ("Message: %s from %s.\n",
                     gst_message_type_get_name (GST_MESSAGE_TYPE (message)),
                     GST_OBJECT_NAME (message->src));
          }
        break;
      }

    }

  /* We handled the messages we wanted, and ignored the ones we didn't want,
   * so the core can unref the message for us. */
  return TRUE;
}
