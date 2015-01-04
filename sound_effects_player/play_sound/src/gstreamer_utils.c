/*
 * gstreamer_utils.c
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
#include <gstreamer_utils.h>
#include <message_handler.h>
#include <math.h>

/* Set up the Gstreamer pipeline. */
GstPipeline *
setup_gstreamer (void *app)
{
  GstElement *source_element, *sink_element, *pan_element, *volume_element;
  GstElement *adder_element;
  GstPipeline *pipeline_element;
  GstElement *bin_element, *final_bin_element;
  GstElement *level_element;
  GstBus *bus;
  gchar *sound_name, *pad_name;
  gint i;
  gdouble frequency;
  GstPad *last_source_pad, *sink_pad;
  GstPadLinkReturn link_status;
  GstStateChangeReturn set_state_val;

  pipeline_element = GST_PIPELINE (gst_pipeline_new ("tones"));
  if (pipeline_element == NULL)
    {
      GST_ERROR ("Unable to create the gstreamer pipeline element.\n");
      return NULL;
    }

  /* We create a bin for each sound effect. */
  for (i = 3; i >= 0; i--)
    {
      /* Create the bin, source and filter elements for sound effect i. */
      frequency = 440.0 * (pow (2.0, ((i + 1.0) / 12.0)));
      sound_name = g_strdup_printf ("tone %d", i);
      bin_element = gst_bin_new (sound_name);
      g_free (sound_name);
      source_element = gst_element_factory_make ("audiotestsrc", "source");
      pan_element = gst_element_factory_make ("audiopanorama", "pan");
      volume_element = gst_element_factory_make ("volume", "volume");
      if ((bin_element == NULL) || (source_element == NULL)
          || (pan_element == NULL) || (volume_element == NULL))
        {
          GST_ERROR
            ("Unable to create the gstreamer sound effect elements.\n");
          return NULL;
        }

      /* Set initial values of the frequency, pan and volume */
      g_object_set (source_element, "freq", frequency, NULL);
      g_object_set (pan_element, "panorama", 0.0, NULL);
      g_object_set (volume_element, "volume", 0.125, NULL);

      /* The bin is muted until it starts to play. */
      g_object_set (volume_element, "mute", TRUE, NULL);

      /* Place the source, pan, and volume elements in the bin. */
      gst_bin_add_many (GST_BIN (bin_element), source_element, pan_element,
                        volume_element, NULL);

      /* Link them together in this order: source->pan->volume. */
      gst_element_link (source_element, pan_element);
      gst_element_link (pan_element, volume_element);

      /* The output of the bin is the output of the last element. */
      last_source_pad = gst_element_get_static_pad (volume_element, "src");
      gst_element_add_pad (bin_element,
                           gst_ghost_pad_new ("src", last_source_pad));

      /* Place the bin in the pipeline. */
      gst_bin_add (GST_BIN (pipeline_element), bin_element);
    }

  /* Create one more bin, to collect the output of the sound effects bins
   * and play them. */
  final_bin_element = gst_bin_new ("final");
  sink_element = gst_element_factory_make ("autoaudiosink", "sink");
  adder_element = gst_element_factory_make ("adder", "adder");
  level_element = gst_element_factory_make ("level", "master level");
  if ((final_bin_element == NULL) || (sink_element == NULL)
      || (adder_element == NULL) || (level_element == NULL))
    {
      GST_ERROR ("Unable to create the final gstreamer elements.\n");
      return NULL;
    }

  /* Put the adder, level and sink into the final bin. */
  gst_bin_add_many (GST_BIN (final_bin_element), adder_element, level_element,
                    sink_element, NULL);

  /* The input to the final bin is the inputs to the adder. */
  for (i = 0; i < 4; i++)
    {
      sink_pad = gst_element_get_request_pad (adder_element, "sink_%u");
      pad_name = g_strdup_printf ("sink %d", i);
      gst_element_add_pad (final_bin_element,
                           gst_ghost_pad_new (pad_name, sink_pad));
      g_free (pad_name);
    }

  /* Link the adder to the level, and the level to the sink. */
  gst_element_link (adder_element, level_element);
  gst_element_link (level_element, sink_element);

  /* Place the final bin in the pipeline. */
  gst_bin_add (GST_BIN (pipeline_element), final_bin_element);

  /* Link the outputs of the sound effects bins to the input of
   * the final bin. */
  for (i = 0; i < 4; i++)
    {
      sound_name = g_strdup_printf ("tone %d", i);
      bin_element =
        gst_bin_get_by_name (GST_BIN (pipeline_element), sound_name);
      if (bin_element == NULL)
        {
          GST_ERROR ("Cannot find bin for sound %s.\n", sound_name);
          return NULL;
        }
      pad_name = g_strdup_printf ("sink %d", i);
      last_source_pad = gst_element_get_static_pad (bin_element, "src");
      sink_pad = gst_element_get_static_pad (final_bin_element, pad_name);
      link_status = gst_pad_link (last_source_pad, sink_pad);
      if (link_status != GST_PAD_LINK_OK)
        {
          GST_ERROR ("Failed to link sound effect to final bin: %d, %d.\n", i,
                     link_status);
        }
      g_free (pad_name);
      g_free (sound_name);
    }

  /* Make sure we will get level messages. */
  g_object_set (level_element, "post-messages", TRUE, NULL);
  bus = gst_element_get_bus (GST_ELEMENT (pipeline_element));
  gst_bus_add_watch (bus, play_sound_message_handler, app);

  /* Now that the pipeline is constructed, start it running.
   * Note that all of the bins providing input to the adder are muted,
   * so there will be no sound until a button is pushed. */
  set_state_val =
    gst_element_set_state (GST_ELEMENT (pipeline_element), GST_STATE_PLAYING);
  if (set_state_val == GST_STATE_CHANGE_FAILURE)
    {
      GST_ERROR ("Unable to start the gstreamer pipeline.\n");
      return NULL;
    }

  /* For debugging, write out a graphical representation of the pipeline. */
  play_sound_debug_dump_pipeline (pipeline_element);

  return pipeline_element;
}

/* For debugging, write out an annotated, graphical representation
 * of the gstreamer pipeline.
 */
void
play_sound_debug_dump_pipeline (GstPipeline * pipeline_element)
{
  gst_debug_bin_to_dot_file_with_ts (GST_BIN (pipeline_element),
                                     GST_DEBUG_GRAPH_SHOW_ALL,
                                     "play_sound_pipeline");
  return;
}

/* Find a gstreamer bin, given its name. */
GstBin *
play_sound_find_bin (GstPipeline * pipeline_element, gchar * bin_name)
{
  GstElement *bin_element;

  bin_element = gst_bin_get_by_name (GST_BIN (pipeline_element), bin_name);
  return (GST_BIN (bin_element));
}

/* Find the volume control in a bin. */
GstElement *
play_sound_find_volume (GstBin * bin_element)
{
  GstElement *volume_element;

  volume_element = gst_bin_get_by_name (bin_element, "volume");
  return (volume_element);
}

/* We are done with Gstreamer; shut it down. */
void
shutdown_gstreamer (GstPipeline * pipeline_element)
{
  gst_element_set_state (GST_ELEMENT (pipeline_element), GST_STATE_NULL);
  g_object_unref (pipeline_element);
  return;
}
