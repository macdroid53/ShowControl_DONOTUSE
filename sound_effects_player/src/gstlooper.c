/*
 * Show_control, a Gstreamer application
 * Copyright © 2006 Stefan Kost <ensonic@users.sf.net>
 * Copyright © 2015 John Sauter <John_Sauter@systemeyescomputerstore.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, see https://gnu.org/licenses
 * or write to
 * Free Software Foundation, Inc.
 * 51 Franklin Street, Fifth Floor
 * Boston, MA 02111-1301
 * USA.
 */

/**
 * SECTION:element-looper
 *
 * Repeat a section of the input stream a specified number of times.
 * loop-from is the beginning of the section to repeat, in nanoseconds from
 * the beginning of the input.  loop-to is the end of the section to repeat,
 * also in nanoseconds.  loop-limit is the number of times to repeat; 0
 * means repeat indefinitely.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch-1.0 -v -m audiotestsrc ! looper loop_end=1000000 ! fakesink silent=TRUE
 * ]|
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <math.h>
#include <gst/gst.h>
#include <gst/base/gstbasetransform.h>
#include <gst/audio/audio.h>
#include <gst/audio/gstaudiofilter.h>
#include <gst/base/gsttypefindhelper.h>

#include "gstlooper.h"

/* GST_DEBUG_CATEGORY (looper); */
GST_DEBUG_CATEGORY_STATIC (looper);
#define GST_CAT_DEFAULT looper
GST_DEBUG_CATEGORY_STATIC (GST_CAT_PERFORMANCE);

/* Filter signals and args */
enum
{
  /* FILL ME */
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_SILENT,
  PROP_LOOP_TO,
  PROP_LOOP_FROM,
  PROP_LOOP_LIMIT
};

#if G_BYTE_ORDER == G_LITTLE_ENDIAN
#define ALLOWED_CAPS \
	"audio/x-raw, " \
  "format = (string) { F64LE }, " \
  "rate = (int) { 96000, 48000 }, " \
  "channels = (int) [ 1, 2 ]," \
  "layout = (string) { interleaved }"
#else
#define ALLOWED_CAPS \
	"audio/x-raw, " \
  "format = (string) { F64BE }, " \
  "rate = (int) { 96000, 48000 }, " \
  "channels = (int) [ 1, 2 ]," \
  "layout = (string) { interleaved, non-interleaved }"
#endif

#define DEBUG_INIT \
  GST_DEBUG_CATEGORY_INIT (looper, "looper", 0, \
			   "Repeat a section of the stream"); \
  GST_DEBUG_CATEGORY_GET (GST_CAT_PERFORMANCE, "GST_PERFORMANCE");
#define gst_looper_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE (GstLooper, gst_looper, GST_TYPE_AUDIO_FILTER,
                         DEBUG_INIT);

static void gst_looper_set_property (GObject * object, guint prop_id,
                                     const GValue * value,
                                     GParamSpec * pspec);
static void gst_looper_get_property (GObject * object, guint prop_id,
                                     GValue * value, GParamSpec * pspec);

static void looper_before_transform (GstBaseTransform * base,
                                     GstBuffer * buffer);

static GstFlowReturn looper_transform_ip (GstBaseTransform * base,
                                          GstBuffer * outbuf);
static GstFlowReturn looper_transform (GstBaseTransform * base,
                                       GstBuffer * inbuf, GstBuffer * outbuf);

static void
looper_before_transform (GstBaseTransform * base, GstBuffer * buffer)
{
  GstClockTime timestamp, duration;
  GstLooper *self = GST_LOOPER (base);
  GstEvent *seek_event;
  GstPad *sink_pad;
  gboolean doing_loops;

  timestamp = GST_BUFFER_TIMESTAMP (buffer);
  timestamp =
    gst_segment_to_stream_time (&base->segment, GST_FORMAT_TIME, timestamp);
  duration = GST_BUFFER_DURATION (buffer);
  duration =
    gst_segment_to_stream_time (&base->segment, GST_FORMAT_TIME, duration);

  GST_DEBUG_OBJECT (self, "timestamp: %" GST_TIME_FORMAT ".",
                    GST_TIME_ARGS (timestamp));
  GST_DEBUG_OBJECT (self, "duration: %" GST_TIME_FORMAT ".",
                    GST_TIME_ARGS (duration));
  GST_DEBUG_OBJECT (self, "loop_counter: %" G_GUINT32_FORMAT ".",
                    self->loop_counter);

  if (GST_CLOCK_TIME_IS_VALID (timestamp))
    gst_object_sync_values (GST_OBJECT (self), timestamp);

/* Test to see if we are looping.  By default, loop_to, loop_from and
   * loop_limit are all zero.  Unless loop_from is moved forward in time,
   * no looping is done.  Realistically, loop_from must be greater than
   * loop_to, or we will be skipping rather than looping.  Also, if
   * loop_limit has been set non-zero, we stop looping when we have
   * reached the limit.  */
  doing_loops = FALSE;
  if (self->loop_to < self->loop_from)
    {
      if ((self->loop_limit == 0) || (self->loop_counter < self->loop_limit))
        {
          doing_loops = TRUE;
        }
    }

  /* If we have reached the end of the loop, issue a seek
   * upstream to return to its beginning.  */
  if (doing_loops && ((timestamp) >= self->loop_from))
    {
      /* Construct a seek event to return to the loop_to point.  */
      seek_event =
        gst_event_new_seek (1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_ACCURATE,
                            GST_SEEK_TYPE_SET, self->loop_to,
                            GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE);
      /* Find our sink pad.  */
      sink_pad = gst_element_get_static_pad (GST_ELEMENT (self), "sink");
      /* Send the seek event to the sink pad's peer, which is the source
       * pad of the element upstream from us.  He will pass it upstream
       * until it finds an element which can process it, probably a file 
       * reader.  */
      gst_pad_push_event (sink_pad, seek_event);
      GST_DEBUG_OBJECT (self, "seek to %" G_GUINT64_FORMAT ".",
                        self->loop_to);

      /* We are done with the sink pad.  */
      gst_object_unref (sink_pad);

      /* Count the number of times we have looped, in case there is a limit.  */
      self->loop_counter = self->loop_counter + 1;
      GST_DEBUG_OBJECT (self, "loop counter is %d.", self->loop_counter);

      /* Compute the timestamp offset caused by looping.  */
      self->loop_duration = self->loop_from - self->loop_to;
      self->timestamp_offset = self->loop_duration * self->loop_counter;
      GST_DEBUG_OBJECT (self, "timestamp offset is %" G_GUINT64_FORMAT ".",
                        self->timestamp_offset);

    }

}

static GstFlowReturn
looper_transform_ip (GstBaseTransform * base, GstBuffer * outbuf)
{
  GstAudioFilter *filter = GST_AUDIO_FILTER_CAST (base);
  GstLooper *self = GST_LOOPER (base);
  GstMapInfo map;
  GstClockTime ts;
  gint rate = GST_AUDIO_INFO_RATE (&filter->info);
  gint width = GST_AUDIO_FORMAT_INFO_WIDTH (filter->info.finfo);
  gint channels = GST_AUDIO_INFO_CHANNELS (&filter->info);
  guint nsamples;
  GstClockTime interval = gst_util_uint64_scale_int (1, GST_SECOND, rate);

  /* Don't process data with GAP.  */
  if (GST_BUFFER_FLAG_IS_SET (outbuf, GST_BUFFER_FLAG_GAP))
    return GST_FLOW_OK;

  gst_buffer_map (outbuf, &map, GST_MAP_READWRITE);
  nsamples = map.size / (width * channels / 8);
  ts = GST_BUFFER_TIMESTAMP (outbuf);
  ts = gst_segment_to_stream_time (&base->segment, GST_FORMAT_TIME, ts);

  GST_DEBUG_OBJECT (self,
                    "transform in place timestamp: %" GST_TIME_FORMAT ".",
                    GST_TIME_ARGS (ts));
  GST_DEBUG_OBJECT (self, "interval: %" GST_TIME_FORMAT ".",
                    GST_TIME_ARGS (interval));
  GST_DEBUG_OBJECT (self, "rate: %d, width: %d, channels: %d, samples: %d.",
                    rate, width, channels, nsamples);

  /* Here process data.  Looper does no processing.  */

  gst_buffer_unmap (outbuf, &map);
  return GST_FLOW_OK;
};

static GstFlowReturn
looper_transform (GstBaseTransform * base, GstBuffer * inbuf,
                  GstBuffer * outbuf)
{
  GstAudioFilter *filter = GST_AUDIO_FILTER_CAST (base);
  GstLooper *self = GST_LOOPER (base);
  GstMapInfo srcmap, dstmap;
  gint insize, outsize;
  gboolean inbuf_writable;
  gint sample_count;
  gdouble *src, *dst;
  gint copy_counter;
  GstClockTime ts;
  gint rate = GST_AUDIO_INFO_RATE (&filter->info);
  gint width = GST_AUDIO_FORMAT_INFO_WIDTH (filter->info.finfo);
  gint channels = GST_AUDIO_INFO_CHANNELS (&filter->info);
  GstClockTime interval = gst_util_uint64_scale_int (1, GST_SECOND, rate);

  /* Get the number of samples to process.  */
  sample_count = gst_buffer_get_size (inbuf) / sizeof (gdouble);

  ts = GST_BUFFER_TIMESTAMP (outbuf);
  ts = gst_segment_to_stream_time (&base->segment, GST_FORMAT_TIME, ts);
  GST_DEBUG_OBJECT (self, "transform timestamp: %" GST_TIME_FORMAT ".",
                    GST_TIME_ARGS (ts));
  GST_DEBUG_OBJECT (self, "interval: %" GST_TIME_FORMAT ".",
                    GST_TIME_ARGS (interval));
  GST_DEBUG_OBJECT (self, "rate: %d, width: %d, channels: %d, samples: %d.",
                    rate, width, channels, sample_count);

  /* Compute the size of the necessary buffer.  */
  insize = sample_count * sizeof (gdouble);
  outsize = sample_count * sizeof (gdouble);

  if (insize == 0 || outsize == 0)
    return GST_FLOW_OK;

  inbuf_writable = gst_buffer_is_writable (inbuf)
    && gst_buffer_n_memory (inbuf) == 1
    && gst_memory_is_writable (gst_buffer_peek_memory (inbuf, 0));

  /* Get the source and destination data.  */
  gst_buffer_map (inbuf, &srcmap,
                  inbuf_writable ? GST_MAP_READWRITE : GST_MAP_READ);
  gst_buffer_map (outbuf, &dstmap, GST_MAP_WRITE);

  /* Check in and out size.  */
  if (srcmap.size < insize)
    {
      GST_ELEMENT_ERROR (self, STREAM, FORMAT, (NULL),
                         ("input buffers are of wrong size: %" G_GSIZE_FORMAT
                          " < %d.", srcmap.size, insize));
      gst_buffer_unmap (outbuf, &dstmap);
      gst_buffer_unmap (inbuf, &srcmap);
      return GST_FLOW_ERROR;
    }

  if (dstmap.size < outsize)
    {
      GST_ELEMENT_ERROR (self, STREAM, FORMAT, (NULL),
                         ("output buffers are of wrong size: %" G_GSIZE_FORMAT
                          " < %d.", dstmap.size, outsize));
      gst_buffer_unmap (outbuf, &dstmap);
      gst_buffer_unmap (inbuf, &srcmap);
      return GST_FLOW_ERROR;
    }

  /* Do nothing with gaps.  */
  if (GST_BUFFER_FLAG_IS_SET (inbuf, GST_BUFFER_FLAG_GAP))
    {
      gst_buffer_unmap (outbuf, &dstmap);
      gst_buffer_unmap (inbuf, &srcmap);
      return GST_FLOW_OK;
    }

  /* Copy the samples.  */
  src = (gdouble *) srcmap.data;
  dst = (gdouble *) dstmap.data;

  GST_DEBUG_OBJECT (self, "copy %d samples.", sample_count * channels);
  for (copy_counter = 0; copy_counter < (sample_count * channels);
       copy_counter++)
    {
      /* GST_LOG_OBJECT (self, "copy from address %p to %p, value %f.",
	 src, dst, *src); */
      *dst++ = *src++;
    }

  return GST_FLOW_OK;
}


static gboolean
looper_stop (GstBaseTransform * base)
{
  /* GstLooper *self = GST_LOOPER (base); */
  return GST_CALL_PARENT_WITH_DEFAULT (GST_BASE_TRANSFORM_CLASS, stop, (base),
                                       TRUE);
};

static gboolean
looper_setup (GstAudioFilter * filter, const GstAudioInfo * info)
{
  GstLooper *self = GST_LOOPER (filter);
  GST_OBJECT_LOCK (self);
  GST_OBJECT_UNLOCK (self);
  return TRUE;
};

static void
gst_looper_dispose (GObject * object)
{
  /* GstLooper *self = GST_LOOPER (object); */
  G_OBJECT_CLASS (parent_class)->dispose (object);
};

/* GObject vmethod implementations */

/* initialize the looper's class */
static void
gst_looper_class_init (GstLooperClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstBaseTransformClass *trans_class;
  GstAudioFilterClass *filter_class;
  GstCaps *caps;
  GParamSpec *param_spec;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  trans_class = (GstBaseTransformClass *) klass;
  filter_class = (GstAudioFilterClass *) (klass);

  gobject_class->set_property = gst_looper_set_property;
  gobject_class->get_property = gst_looper_get_property;
  gobject_class->dispose = gst_looper_dispose;

  param_spec =
    g_param_spec_boolean ("silent", "Silent", "Produce verbose output ?",
                          FALSE, G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE);
  g_object_class_install_property (gobject_class, PROP_SILENT, param_spec);

  param_spec =
    g_param_spec_uint64 ("loop-to", "Loop_to", "Start of section to repeat",
                         0, G_MAXUINT64, 0, G_PARAM_READWRITE);
  g_object_class_install_property (gobject_class, PROP_LOOP_TO, param_spec);

  param_spec =
    g_param_spec_uint64 ("loop-from", "Loop_from", "End of section to repeat",
                         0, G_MAXUINT64, 0, G_PARAM_READWRITE);
  g_object_class_install_property (gobject_class, PROP_LOOP_FROM, param_spec);

  param_spec =
    g_param_spec_uint ("loop-limit", "Loop_limit",
                       "Number of times to repeat; 0 means forever", 0,
                       G_MAXUINT, 0, G_PARAM_READWRITE);

  g_object_class_install_property (gobject_class, PROP_LOOP_LIMIT,
                                   param_spec);

  gst_element_class_set_static_metadata (gstelement_class, "Looper",
                                         "Filter/Effect/Audio",
                                         "Repeat a section of the input stream",
                                         "John Sauter <John_Sauter@"
                                         "systemeyescomputerstore.com>");

  caps = gst_caps_from_string (ALLOWED_CAPS);
  gst_audio_filter_class_add_pad_templates (filter_class, caps);
  gst_caps_unref (caps);

  trans_class->before_transform = GST_DEBUG_FUNCPTR (looper_before_transform);
  trans_class->transform_ip = GST_DEBUG_FUNCPTR (looper_transform_ip);
  trans_class->transform = GST_DEBUG_FUNCPTR (looper_transform);
  trans_class->stop = GST_DEBUG_FUNCPTR (looper_stop);
  trans_class->transform_ip_on_passthrough = FALSE;

  filter_class->setup = looper_setup;
}

/* initialize the new element
 * initialize instance structure
 */
static void
gst_looper_init (GstLooper * self)
{

  self->silent = FALSE;
  self->loop_from = 0;
  self->loop_from = 0;
  self->loop_limit = 0;
  self->loop_counter = 0;
  self->loop_duration = 0;
  self->timestamp_offset = 0;
}

static void
gst_looper_set_property (GObject * object, guint prop_id,
                         const GValue * value, GParamSpec * pspec)
{
  GstLooper *self = GST_LOOPER (object);

  switch (prop_id)
    {
    case PROP_SILENT:
      GST_OBJECT_LOCK (self);
      self->silent = g_value_get_boolean (value);
      GST_OBJECT_UNLOCK (self);
      break;

    case PROP_LOOP_TO:
      GST_OBJECT_LOCK (self);
      self->loop_to = g_value_get_uint64 (value);
      GST_DEBUG_OBJECT (self, "loop-to: %" G_GUINT64_FORMAT ".",
                        self->loop_to);
      GST_OBJECT_UNLOCK (self);
      break;

    case PROP_LOOP_FROM:
      GST_OBJECT_LOCK (self);
      self->loop_from = g_value_get_uint64 (value);
      GST_DEBUG_OBJECT (self, "loop-from: %" G_GUINT64_FORMAT ".",
                        self->loop_from);
      GST_OBJECT_UNLOCK (self);
      break;

    case PROP_LOOP_LIMIT:
      GST_OBJECT_LOCK (self);
      self->loop_limit = g_value_get_uint (value);
      GST_DEBUG_OBJECT (self, "loop-limit: %" G_GUINT32_FORMAT ".",
                        self->loop_limit);
      GST_OBJECT_UNLOCK (self);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gst_looper_get_property (GObject * object, guint prop_id, GValue * value,
                         GParamSpec * pspec)
{
  GstLooper *self = GST_LOOPER (object);

  switch (prop_id)
    {
    case PROP_SILENT:
      GST_OBJECT_LOCK (self);
      g_value_set_boolean (value, self->silent);
      GST_OBJECT_UNLOCK (self);
      break;
    case PROP_LOOP_TO:
      GST_OBJECT_LOCK (self);
      g_value_set_uint64 (value, self->loop_to);
      GST_OBJECT_UNLOCK (self);
      break;
    case PROP_LOOP_FROM:
      GST_OBJECT_LOCK (self);
      g_value_set_uint64 (value, self->loop_from);
      GST_OBJECT_UNLOCK (self);
      break;
    case PROP_LOOP_LIMIT:
      GST_OBJECT_LOCK (self);
      g_value_set_uint (value, self->loop_limit);
      GST_OBJECT_UNLOCK (self);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean
looper_init (GstPlugin * looper)
{
  return gst_element_register (looper, "looper", GST_RANK_NONE,
                               GST_TYPE_LOOPER);
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR, GST_VERSION_MINOR, looper,
                   "Repeat a secction of the input stream", looper_init,
                   VERSION, "LGPL", "GStreamer", "http://gstreamer.net/")
