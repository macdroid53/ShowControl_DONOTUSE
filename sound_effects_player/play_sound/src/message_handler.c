/*
 * message_handler.c
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

#include <message_handler.h>
#include <math.h>
#include <gst/gst.h>
#include <update_display.h>

/* Process a message from the pipeline. User_data is the 
 * application, so we can reach the display.  */
gboolean
play_sound_message_handler (GstBus * bus_element, GstMessage * message,
                            gpointer user_data)
{
  /* We care only about messages from the level element, which show
   * the sound level on each channel. */
  if (message->type == GST_MESSAGE_ELEMENT)
    {
      const GstStructure *s = gst_message_get_structure (message);
      const gchar *name = gst_structure_get_name (s);

      if (g_ascii_strcasecmp (name, "level") == 0)
        {
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
              update_vu_meter (user_data, i, rms);
            }
        }
    }
  /* We handled the message we wanted, and ignored the ones we didn't want,
   * so the core can unref the message for us. */
  return TRUE;
}
