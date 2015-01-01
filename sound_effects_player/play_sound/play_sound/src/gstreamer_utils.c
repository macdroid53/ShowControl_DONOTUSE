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

/* Set up the Gstreamer pipeline. */
GstElement *
setup_gstreamer (void)
{
  GstElement *source_element, *sink_element, *pan_element, *volume_element;
  GstElement *pipeline_element;

  pipeline_element = gst_pipeline_new ("tone");
  /* Create the source, sink and filter elements. */
  source_element = gst_element_factory_make ("audiotestsrc", "source");
  sink_element = gst_element_factory_make ("autoaudiosink", "output");
  pan_element = gst_element_factory_make ("audiopanorama", "pan");
  volume_element = gst_element_factory_make ("volume", "volume");
  if ((pipeline_element == NULL) || (source_element == NULL)
      || (sink_element == NULL) || (pan_element == NULL)
      || (volume_element == NULL))
    {
      GST_ERROR ("Unable to create the gstreamer elements.\n");
      return NULL;
    }

  /* Set initial values of the frequency, pan and volume */
  g_object_set (source_element, "freq", 440.0, NULL);
  g_object_set (pan_element, "panorama", 0.0, NULL);
  g_object_set (volume_element, "volume", 1.0, NULL);

  /* Place the source, pan, volume and sink elements in the pipeline. */
  gst_bin_add_many (GST_BIN (pipeline_element), source_element, pan_element,
		    volume_element, sink_element, NULL);

  /* Link them together in this order: source->pan->volume->sink. */
  gst_element_link (source_element, pan_element);
  gst_element_link (pan_element, volume_element);
  gst_element_link (volume_element, sink_element);

  /* The pipeline generates no sound until its button is pushed. */
  gst_element_set_state (pipeline_element, GST_STATE_PAUSED);

  return pipeline_element;
}

/* We are done with Gstreamer; shut it down. */
void
shutdown_gstreamer (GstElement * pipeline)
{
  gst_element_set_state (pipeline, GST_STATE_NULL);
  g_object_unref (pipeline);
  return;
}
