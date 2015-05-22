/*
 * gstreamer_subroutines.c
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
#include "gstreamer_subroutines.h"
#include "message_subroutines.h"
#include "sound_structure.h"
#include "sound_effects_player.h"
#include "sound_subroutines.h"
#include "button_subroutines.h"
#include <math.h>

/* Set up the Gstreamer pipeline. */
GstPipeline *
gstreamer_init (int sound_count, GApplication * app)
{
  GstElement *sink_element;
  GstElement *adder_element;
  GstElement *convert_element;
  GstElement *resample_element;
  GstElement *final_bin_element;
  GstElement *level_element;
  GstPipeline *pipeline_element;
  GstBus *bus;
  gchar *pad_name;
  gint i;
  GstPad *sink_pad;

  pipeline_element = GST_PIPELINE (gst_pipeline_new ("sound_effects"));
  if (pipeline_element == NULL)
    {
      GST_ERROR ("Unable to create the gstreamer pipeline element.\n");
      return NULL;
    }

  /* Create the final bin, to collect the output of the sound effects bins
   * and play them. */
  final_bin_element = gst_bin_new ("final");
  sink_element = gst_element_factory_make ("alsasink", "final/sink");
  adder_element = gst_element_factory_make ("adder", "final/adder");
  level_element = gst_element_factory_make ("level", "final/master_level");
  convert_element =
    gst_element_factory_make ("audioconvert", "final/convert");
  resample_element =
    gst_element_factory_make ("audioresample", "final/resample");
  if ((final_bin_element == NULL) || (sink_element == NULL)
      || (adder_element == NULL) || (level_element == NULL)
      || (convert_element == NULL) || (resample_element == NULL))
    {
      GST_ERROR ("Unable to create the final gstreamer elements.\n");
      return NULL;
    }

  /* Put the adder, level, converter, resampler and sink into the final bin. */
  gst_bin_add_many (GST_BIN (final_bin_element), adder_element, level_element,
                    convert_element, resample_element, sink_element, NULL);

  /* Make sure we will get level messages. */
  g_object_set (level_element, "post-messages", TRUE, NULL);

  /* Watch for messages from the pipeline.  */
  bus = gst_element_get_bus (GST_ELEMENT (pipeline_element));
  gst_bus_add_watch (bus, message_handler, app);

  /* The inputs to the final bin are the inputs to the adder.  Create enough
   * sinks for each sound effect.  */
  for (i = 0; i < sound_count; i++)
    {
      sink_pad = gst_element_get_request_pad (adder_element, "sink_%u");
      pad_name = g_strdup_printf ("sink %d", i);
      gst_element_add_pad (final_bin_element,
                           gst_ghost_pad_new (pad_name, sink_pad));
      g_free (pad_name);
    }

  /* Link the various elements in the final bin together.  */
  gst_element_link (adder_element, level_element);
  gst_element_link (level_element, convert_element);
  gst_element_link (convert_element, resample_element);
  gst_element_link (resample_element, sink_element);

  /* Place the final bin in the pipeline. */
  gst_bin_add (GST_BIN (pipeline_element), final_bin_element);

  return pipeline_element;
}

/* Create a Gstreamer bin for a sound effect.  */
GstBin *
gstreamer_create_bin (struct sound_info * sound_data, int sound_number,
                      GstPipeline * pipeline_element, GApplication * app)
{
  GstElement *source_element, *parse_element, *looper_element;
  GstElement *envelope_element, *pan_element, *volume_element;
  GstElement *bin_element, *final_bin_element;
  gchar *sound_name, *pad_name, *element_name;
  GstPad *last_source_pad, *sink_pad;
  GstPadLinkReturn link_status;
  gboolean success;
  gchar string_buffer[G_ASCII_DTOSTR_BUF_SIZE];

  /* Create the bin, source and various filter elements for this sound effect. 
   */
  sound_name = g_strconcat ((gchar *) "sound/", sound_data->name, NULL);
  bin_element = gst_bin_new (sound_name);
  element_name = g_strconcat (sound_name, (gchar *) "/source", NULL);
  source_element = gst_element_factory_make ("filesrc", element_name);
  g_free (element_name);
  element_name = g_strconcat (sound_name, (gchar *) "/parse", NULL);
  parse_element = gst_element_factory_make ("wavparse", element_name);
  g_free (element_name);
  element_name = g_strconcat (sound_name, (gchar *) "/looper", NULL);
  looper_element = gst_element_factory_make ("looper", element_name);
  g_free (element_name);
  element_name = g_strconcat (sound_name, (gchar *) "/envelope", NULL);
  envelope_element = gst_element_factory_make ("envelope", element_name);
  g_free (element_name);
  element_name = g_strconcat (sound_name, (gchar *) "/pan", NULL);
  pan_element = gst_element_factory_make ("audiopanorama", element_name);
  g_free (element_name);
  element_name = g_strconcat (sound_name, (gchar *) "/volume", NULL);
  volume_element = gst_element_factory_make ("volume", element_name);
  g_free (element_name);
  g_free (sound_name);
  element_name = NULL;
  sound_name = NULL;
  if ((bin_element == NULL) || (source_element == NULL)
      || (parse_element == NULL) || (looper_element == NULL)
      || (envelope_element == NULL) || (pan_element == NULL)
      || (volume_element == NULL))
    {
      GST_ERROR
        ("Unable to create all the gstreamer sound effect elements.\n");
      return NULL;
    }

  /* Set parameter values of the elements.  */
  g_object_set (source_element, "location", sound_data->wav_file_name_full,
                NULL);

  g_object_set (looper_element, "loop-to", sound_data->loop_to_time, NULL);
  g_object_set (looper_element, "loop-from", sound_data->loop_from_time,
                NULL);
  g_object_set (looper_element, "loop-limit", sound_data->loop_limit, NULL);

  g_object_set (envelope_element, "attack-duration-time",
                sound_data->attack_duration_time, NULL);
  g_object_set (envelope_element, "attack_level", sound_data->attack_level,
                NULL);
  g_object_set (envelope_element, "decay-duration-time",
                sound_data->decay_duration_time, NULL);
  g_object_set (envelope_element, "sustain-level", sound_data->sustain_level,
                NULL);
  g_object_set (envelope_element, "release-start-time",
                sound_data->release_start_time, NULL);
  if (sound_data->release_duration_infinite)
    {
      g_object_set (envelope_element, "release-duration-time",
                    (gchar *) "∞", NULL);
    }
  else
    {
      g_ascii_dtostr (string_buffer, G_ASCII_DTOSTR_BUF_SIZE,
                      (gdouble) sound_data->release_duration_time);
      g_object_set (envelope_element, "release-duration-time", string_buffer,
                    NULL);
    }
  g_object_set (envelope_element, "volume", sound_data->designer_volume_level,
                NULL);
  g_object_set (envelope_element, "sound-name", sound_data->name, NULL);

  g_object_set (pan_element, "panorama", sound_data->designer_pan, NULL);

  /* Place the various elements in the bin. */
  gst_bin_add_many (GST_BIN (bin_element), source_element, parse_element,
                    looper_element, envelope_element, pan_element,
                    volume_element, NULL);

  /* Link them together in this order: source->parse->looper->envelope->pan->
   * volume. */
  gst_element_link (source_element, parse_element);
  gst_element_link (parse_element, looper_element);
  gst_element_link (looper_element, envelope_element);
  gst_element_link (envelope_element, pan_element);
  gst_element_link (pan_element, volume_element);

  /* The output of the bin is the output of the last element. */
  last_source_pad = gst_element_get_static_pad (volume_element, "src");
  gst_element_add_pad (bin_element,
                       gst_ghost_pad_new ("src", last_source_pad));

  /* Place the bin in the pipeline. */
  success = gst_bin_add (GST_BIN (pipeline_element), bin_element);
  if (!success)
    {
      GST_ERROR ("Failed to add sound effect %s bin to pipeline.\n",
                 sound_data->name);
    }

  /* Link the output of the sound effect bin to the final bin. */
  final_bin_element =
    gst_bin_get_by_name (GST_BIN (pipeline_element), (gchar *) "final");
  pad_name = g_strdup_printf ("sink %d", sound_number);
  last_source_pad = gst_element_get_static_pad (bin_element, "src");
  sink_pad = gst_element_get_static_pad (final_bin_element, pad_name);
  link_status = gst_pad_link (last_source_pad, sink_pad);
  if (link_status != GST_PAD_LINK_OK)
    {
      GST_ERROR ("Failed to link sound effect %s to final bin: %d, %d.\n",
                 sound_data->name, sound_number, link_status);
    }
  g_free (pad_name);

  return (GST_BIN (bin_element));
}

/* After the individual bins are created, complete the pipeline.  */
void
gstreamer_complete_pipeline (GstPipeline * pipeline_element,
                             GApplication * app)
{
  GstStateChangeReturn set_state_val;
  GstBus *bus;
  GstMessage *msg;
  GError *err = NULL;

  /* Now that the pipeline is constructed, start it running.  There will be no
   * sound until a sound effect bin receives a start message.  */
  set_state_val =
    gst_element_set_state (GST_ELEMENT (pipeline_element), GST_STATE_PLAYING);
  if (set_state_val == GST_STATE_CHANGE_FAILURE)
    {
      g_print ("Unable to initial start the gstreamer pipeline.\n");

      /* Check for an error message with details on the bus.  */
      bus = gst_pipeline_get_bus (pipeline_element);
      msg = gst_bus_poll (bus, GST_MESSAGE_ERROR, 0);
      if (msg != NULL)
        {
          gst_message_parse_error (msg, &err, NULL);
          g_print ("Error: %s.\n", err->message);
          g_error_free (err);
          gst_message_unref (msg);
        }
    }

  /* For debugging, write out a graphical representation of the pipeline. */
  gstreamer_dump_pipeline (pipeline_element);

  return;
}

/* We are done with Gstreamer; shut it down. */
void
gstreamer_shutdown (GstPipeline * pipeline_element)
{
  if (pipeline_element != NULL)
    {
      gst_element_set_state (GST_ELEMENT (pipeline_element), GST_STATE_NULL);
      g_object_unref (pipeline_element);
    }

  return;
}

/* Handle the async-done event from the gstreamer pipeline.  */
void
gstreamer_async_done (GApplication * app)
{
  GstPipeline *pipeline_element;

  pipeline_element = sep_get_pipeline_from_app (app);
  gstreamer_dump_pipeline (pipeline_element);
  return;
}

/* The pipeline has reached end of stream.  This shouldn't happen, because
 * the looper will just output silence if it runs out of buffer.
 */
void
gstreamer_process_eos (GApplication * app)
{
  GstPipeline *pipeline_element;
  GstStateChangeReturn set_state_val;
  GstBus *bus;
  GstMessage *msg;
  GError *err = NULL;
  GList *sound_list;
  struct sound_info *sound_data;
  GList *l;

  pipeline_element = sep_get_pipeline_from_app (app);
  gstreamer_dump_pipeline (pipeline_element);

  /* Stop playing all the sounds, and reset the clusters.  */
  sound_list = sep_get_sound_list (app);
  for (l = sound_list; l != NULL; l = l->next)
    {
      sound_data = l->data;
      if (!sound_data->disabled)
        {
          sound_stop_playing (sound_data, app);
          button_reset_cluster (sound_data, app);
        }
    }

  /* Place the pipeline, and all its contents, in the Ready state.  */
  set_state_val =
    gst_element_set_state (GST_ELEMENT (pipeline_element), GST_STATE_READY);
  if (set_state_val == GST_STATE_CHANGE_FAILURE)
    {
      g_print ("Unable to ready the gstreamer pipeline.\n");

      /* Check for an error message with details on the bus.  */
      bus = gst_pipeline_get_bus (pipeline_element);
      msg = gst_bus_poll (bus, GST_MESSAGE_ERROR, 0);
      if (msg != NULL)
        {
          gst_message_parse_error (msg, &err, NULL);
          g_print ("Error: %s.\n", err->message);
          g_error_free (err);
          gst_message_unref (msg);
        }
    }

  return;
}

/* Find the volume control in a bin. */
GstElement *
gstreamer_get_volume (GstBin * bin_element)
{
  GstElement *volume_element;
  gchar *element_name, *bin_name;

  bin_name = gst_element_get_name (bin_element);
  element_name = g_strconcat (bin_name, (gchar *) "/volume", NULL);
  g_free (bin_name);
  volume_element = gst_bin_get_by_name (bin_element, element_name);
  g_free (element_name);

  return (volume_element);
}

/* Find the pan control in a bin. */
GstElement *
gstreamer_get_pan (GstBin * bin_element)
{
  GstElement *pan_element;
  gchar *element_name, *bin_name;

  bin_name = gst_element_get_name (bin_element);
  element_name = g_strconcat (bin_name, (gchar *) "/pan", NULL);
  g_free (bin_name);
  pan_element = gst_bin_get_by_name (bin_element, element_name);
  g_free (element_name);

  return (pan_element);
}

/* For debugging, write out an annotated, graphical representation
 * of the gstreamer pipeline.
 */
void
gstreamer_dump_pipeline (GstPipeline * pipeline_element)
{
  gst_debug_bin_to_dot_file_with_ts (GST_BIN (pipeline_element),
                                     GST_DEBUG_GRAPH_SHOW_ALL,
                                     "sound_effects_player_pipeline");
  return;
}
