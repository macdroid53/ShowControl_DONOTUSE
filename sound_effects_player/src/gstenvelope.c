/*
 * File: gstenvelope.c, part of Show_control, a Gstreamer application
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
 * SECTION:element-envelope
 *
 * Shape the volume of the incoming sound based on a traditional
 * attack, decay, sustain, release amplitude envelope.  Properties
 * are:
 *
 * #GstEnvelope:attack-duration-time is the initial ramp up time from volume 0
 * in nanoseconds.  Default is 0.
 *
 * #GstEnvelope:attack-level is the volume level at the end of the attack.
 * Default is 1.0, which leaves the incoming volume unchanged.
 *
 * #GstEnvelope:decay-duration-time is the time to decay after the attack is 
 * complete, in nanoseconds.  Default is 0.
 *
 * #GstEnvelope:sustain-level is the volume level at the end of the decay.
 * Default is 1.0, which leaves the incoming volume unchanged.
 *
 * #GstEnvelope:release-start-time is the time when the release part of the
 * envelope starts, in nanoseconds.  This value can be changed to the current
 * time using a custom Release event, or by the receipt of EOS from upstream.  
 * The default is 0, which, if left unchanged, disables release.
 *
 * #GstEnvelope:release-duration-time is the length of time for the
 * volume to ramp down to 0 once the release is initiated.  This value
 * is in nanoseconds, but can also be specified as infinity by setting
 * the value to the UTF-8 string for the Unicode character for infinity,
 * U+221E, which is ∞.  Default is 0.
 *
 * #GstEnvelope:volume is the normal volume of the sound; the envelope
 * is scaled by this amount.  It might be used to implement the note-on
 * velocity from a musical instrument.  Default is 1.0.
 *
 * If all the properties are defaulted, and release is never signaled,
 * this audio filter does not change the sound passing through it.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch-1.0 -v -m audiotestsrc ! envelope attack-duration-time=1000000000 ! fakesink silent=TRUE
 * ]| Instead of starting the test tone at full volume, fade it in over one
 * second.
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

#include "gstenvelope.h"

GST_DEBUG_CATEGORY_STATIC (envelope);
#define GST_CAT_DEFAULT envelope
GST_DEBUG_CATEGORY_STATIC (GST_CAT_PERFORMANCE);

/* Filter signals and args */
enum
{
  /* FILL ME */
  RELEASE_SIGNAL,               /* initiate release process */
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_SILENT,
  PROP_ATTACK_DURATION_TIME,
  PROP_ATTACK_LEVEL,
  PROP_DECAY_DURATION_TIME,
  PROP_SUSTAIN_LEVEL,
  PROP_RELEASE_START_TIME,
  PROP_RELEASE_DURATION_TIME,
  PROP_VOLUME
};

/* For simplicity, we handle only floating point samples.
 * If there is a need for conversion to another type, it can be
 * done using audioconvert.  */

#if G_BYTE_ORDER == G_LITTLE_ENDIAN
#define ALLOWED_CAPS \
	"audio/x-raw, " \
  "format = (string) { F64LE, F32LE }, " \
  "rate = (int) [ 1, 2147483647 ], " \
  "channels = (int) [ 1, 32 ]," \
  "layout = (string) { interleaved }"
#else
#define ALLOWED_CAPS \
	"audio/x-raw, " \
  "format = (string) { F64BE, F32BE }, " \
  "rate = (int) [ 1, 2147483647 ], " \
  "channels = (int) [ 1, 32 ]," \
  "layout = (string) { interleaved }"
#endif

#define DEBUG_INIT \
  GST_DEBUG_CATEGORY_INIT (envelope, "envelope", 0, \
			   "Shape the amplitude of the sound"); \
  GST_DEBUG_CATEGORY_GET (GST_CAT_PERFORMANCE, "GST_PERFORMANCE");
#define gst_envelope_parent_class parent_class

/* By basing this filter on GstAudioFilter, we impose the requirement
 * that the output is in the same format as the input.  */
G_DEFINE_TYPE_WITH_CODE (GstEnvelope, gst_envelope, GST_TYPE_AUDIO_FILTER,
                         DEBUG_INIT);

/* Forward declarations.  These subroutines will be defined below.  */
static void gst_envelope_set_property (GObject * object, guint prop_id,
                                       const GValue * value,
                                       GParamSpec * pspec);
static void gst_envelope_get_property (GObject * object, guint prop_id,
                                       GValue * value, GParamSpec * pspec);

static void envelope_before_transform (GstBaseTransform * base,
                                       GstBuffer * buffer);

static GstFlowReturn envelope_transform_ip (GstBaseTransform * base,
                                            GstBuffer * outbuf);
static GstFlowReturn envelope_transform (GstBaseTransform * base,
                                         GstBuffer * inbuf,
                                         GstBuffer * outbuf);

static gdouble compute_volume (GstEnvelope * self, GstClockTime timestamp);

/* Before each transform of input to output, do this.  */
static void
envelope_before_transform (GstBaseTransform * base, GstBuffer * buffer)
{
  GstClockTime timestamp, duration;
  GstEnvelope *self = GST_ENVELOPE (base);

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

  if (GST_CLOCK_TIME_IS_VALID (timestamp))
    gst_object_sync_values (GST_OBJECT (self), timestamp);
}

/* Convert input data to output data, using the same buffer for
 * input and output.  That is, the data is modified in place.  */
static GstFlowReturn
envelope_transform_ip (GstBaseTransform * base, GstBuffer * outbuf)
{
  GstAudioFilter *filter = GST_AUDIO_FILTER_CAST (base);
  GstEnvelope *self = GST_ENVELOPE (base);
  GstMapInfo map;
  GstClockTime ts;
  gint rate = GST_AUDIO_INFO_RATE (&filter->info);
  gint width = GST_AUDIO_FORMAT_INFO_WIDTH (filter->info.finfo);
  gint channel_count = GST_AUDIO_INFO_CHANNELS (&filter->info);
  gint frame_count;
  gint frame_counter, channel_counter;
  gdouble volume_val;
  gdouble *src64;
  gfloat *src32;
  GstClockTime interval = gst_util_uint64_scale_int (1, GST_SECOND, rate);

  /* Don't process data with GAP.  */
  if (GST_BUFFER_FLAG_IS_SET (outbuf, GST_BUFFER_FLAG_GAP))
    return GST_FLOW_OK;

  /* Map the buffer as read-write, since the modifications are performed
   * in place.  */
  gst_buffer_map (outbuf, &map, GST_MAP_READWRITE);

  /* Compute the number of frames.  Each frame takes one time step and has
   * a sample for each channel.  Note that the width is in bits.  */
  frame_count = gst_buffer_get_size (outbuf) / (width * channel_count / 8);

  ts = GST_BUFFER_TIMESTAMP (outbuf);
  ts = gst_segment_to_stream_time (&base->segment, GST_FORMAT_TIME, ts);

  GST_DEBUG_OBJECT (self,
                    "transform in place timestamp: %" GST_TIME_FORMAT ".",
                    GST_TIME_ARGS (ts));
  GST_DEBUG_OBJECT (self, "interval: %" GST_TIME_FORMAT ".",
                    GST_TIME_ARGS (interval));
  GST_DEBUG_OBJECT (self, "rate: %d, width: %d, channels: %d, frames: %d.",
                    rate, width, channel_count, frame_count);

  /* For each frame, compute the volume adjustment and apply it to
   * each sample.  There will be one sample per channel.  */

  src64 = (gdouble *) map.data;
  src32 = (gfloat *) map.data;
  for (frame_counter = 0; frame_counter < frame_count; frame_counter++)
    {
      volume_val = compute_volume (self, ts);
      for (channel_counter = 0; channel_counter < channel_count;
           channel_counter++)
        {
          /* Since we only allow floating-point, we can use the width
           * to determine the data type.  32 bits is gfloat and 64 bits
           * is gdouble.  */
          switch (width)
            {
            case 64:
              GST_LOG_OBJECT (self, "sample with value %g becomes %g.",
                              *src64, volume_val * *src64);
              *src64 = volume_val * *src64;
              src64++;
              break;

            case 32:
              GST_LOG_OBJECT (self, "sample with value %g becomes %g.",
                              *src32, volume_val * *src32);
              *src32 = volume_val * *src32;
              src32++;
              break;

            default:
              GST_ELEMENT_ERROR (self, STREAM, FORMAT, (NULL),
                                 ("unknown sample width: %d.", width));
              break;
            }
        }
      ts = ts + interval;
    }

  /* We are done with the buffer.  */
  gst_buffer_unmap (outbuf, &map);
  return GST_FLOW_OK;
}

/* Convert input data to output data, using different buffers for
 * input and output.  */
static GstFlowReturn
envelope_transform (GstBaseTransform * base, GstBuffer * inbuf,
                    GstBuffer * outbuf)
{
  GstAudioFilter *filter = GST_AUDIO_FILTER_CAST (base);
  GstEnvelope *self = GST_ENVELOPE (base);
  GstMapInfo srcmap, dstmap;
  gint insize, outsize;
  gboolean inbuf_writable;
  gint frame_count;
  gdouble *src64, *dst64;
  gfloat *src32, *dst32;
  gdouble volume_val;
  gint frame_counter, channel_counter;
  GstClockTime ts;
  gint rate = GST_AUDIO_INFO_RATE (&filter->info);
  gint width = GST_AUDIO_FORMAT_INFO_WIDTH (filter->info.finfo);
  gint channel_count = GST_AUDIO_INFO_CHANNELS (&filter->info);
  GstClockTime interval = gst_util_uint64_scale_int (1, GST_SECOND, rate);

  /* Get the number of frames to process.  Each frame has a sample for
   * each channel, and each sample contains "width" bits.  */
  frame_count = gst_buffer_get_size (inbuf) / (width * channel_count / 8);

  ts = GST_BUFFER_TIMESTAMP (outbuf);
  ts = gst_segment_to_stream_time (&base->segment, GST_FORMAT_TIME, ts);
  GST_DEBUG_OBJECT (self, "transform timestamp: %" GST_TIME_FORMAT ".",
                    GST_TIME_ARGS (ts));
  GST_DEBUG_OBJECT (self, "interval: %" GST_TIME_FORMAT ".",
                    GST_TIME_ARGS (interval));
  GST_DEBUG_OBJECT (self, "rate: %d, width: %d, channels: %d, frames: %d.",
                    rate, width, channel_count, frame_count);

  /* Compute the size of the necessary buffers.  */
  insize = frame_count * channel_count * width / 8;
  outsize = frame_count * channel_count * width / 8;

  /* A zero-length buffer has no data to modify.  */
  if (insize == 0 || outsize == 0)
    return GST_FLOW_OK;

  inbuf_writable = gst_buffer_is_writable (inbuf)
    && gst_buffer_n_memory (inbuf) == 1
    && gst_memory_is_writable (gst_buffer_peek_memory (inbuf, 0));

  /* Get pointers to the source and destination data.  */
  gst_buffer_map (inbuf, &srcmap,
                  inbuf_writable ? GST_MAP_READWRITE : GST_MAP_READ);
  gst_buffer_map (outbuf, &dstmap, GST_MAP_WRITE);

  /* Check in and out size.  */
  if (srcmap.size < insize)
    {
      GST_ELEMENT_ERROR (self, STREAM, FORMAT, (NULL),
                         ("input buffer is the wrong size: %" G_GSIZE_FORMAT
                          " < %d.", srcmap.size, insize));
      gst_buffer_unmap (outbuf, &dstmap);
      gst_buffer_unmap (inbuf, &srcmap);
      return GST_FLOW_ERROR;
    }

  if (dstmap.size < outsize)
    {
      GST_ELEMENT_ERROR (self, STREAM, FORMAT, (NULL),
                         ("output buffer is the wrong size: %" G_GSIZE_FORMAT
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

  /* Copy the samples, applying the volume adjustment as we go.  */
  src64 = (gdouble *) srcmap.data;
  dst64 = (gdouble *) dstmap.data;
  src32 = (gfloat *) srcmap.data;
  dst32 = (gfloat *) dstmap.data;

  GST_DEBUG_OBJECT (self, "copy %d values.", frame_count * channel_count);
  for (frame_counter = 0; frame_counter < frame_count; frame_counter++)
    {
      volume_val = compute_volume (self, ts);
      for (channel_counter = 0; channel_counter < channel_count;
           channel_counter++)
        {
          /* Since we only allow floating-point, we can use the width
           * to determine the data type.  32 bits is gfloat and 64 bits
           * is gdouble.  */
          switch (width)
            {
            case 64:
              GST_LOG_OBJECT (self, "sample with value %g becomes %g.",
                              *src64, volume_val * *src64);
              *dst64 = volume_val * *src64;
              src64++;
              dst64++;
              break;

            case 32:
              GST_LOG_OBJECT (self, "sample with value %g becomes %g.",
                              *src32, volume_val * *src32);
              *dst32 = volume_val * *src32;
              src32++;
              dst32++;
              break;

            default:
              GST_ELEMENT_ERROR (self, STREAM, FORMAT, (NULL),
                                 ("unknown sample width: %d.", width));
              break;
            }
        }
      ts = ts + interval;
    }

  /* We are done with the buffers.  */
  gst_buffer_unmap (outbuf, &dstmap);
  gst_buffer_unmap (inbuf, &srcmap);
  return GST_FLOW_OK;
}

/* Compute the volume adjustment for a frame.  */
static gdouble
compute_volume (GstEnvelope * self, GstClockTime ts)
{
  gdouble volume_val;
  enum
  { attack, decay, sustain, release, completed } envelope_position;
  GstClockTime decay_end_time;
  gdouble decay_fraction, release_fraction;

  /* Decide where we are in the amplitude envelope.  The normal progression
   * is attack, decay, sustain, release, completed.  However, a release can 
   * arrive at at any time.  If the decay duration time is 0 we go straight from
   * attack to sustain, so sustain level should equal attack level.  */
  if (ts < self->attack_duration_time)
    {
      /* The attack is not yet complete.  */
      envelope_position = attack;
    }
  else
    {
      if (ts < self->attack_duration_time + self->decay_duration_time)
        {
          /* The decay is not yet complete.  */
          envelope_position = decay;
        }
      else
        {
          if ((ts < self->release_start_time)
              || (self->release_start_time == 0))
            {
              /* The decay is complete but we have not yet started release.  */
              envelope_position = sustain;
            }
          else
            {
              if (self->release_duration_infinite
                  || ts <
                  (self->release_start_time + self->release_duration_time))
                {
                  /* The release section of the envelope is running.  */
                  envelope_position = release;
                  if (!self->release_triggered)
                    {
                      /* This is the beginning of the release.  */
                      self->release_triggered = TRUE;
                      self->release_start_volume = self->last_volume;
                      GST_INFO_OBJECT (self,
                                       "Release triggered at %"
                                       GST_TIME_FORMAT " with volume %f.",
                                       GST_TIME_ARGS (ts),
                                       self->release_start_volume);
                    }
                }
              else
                {
                  /* The release is complete.  */
                  envelope_position = completed;
                }
            }
        }
    }

  switch (envelope_position)
    {
    case attack:
      /* The initial attack.  We ramp up to the specified attack level,
       * reaching it at the specified attack duration time.  */
      GST_LOG_OBJECT (self, "attack until %" GST_TIME_FORMAT " to %f.",
                      GST_TIME_ARGS (self->attack_duration_time),
                      self->attack_level);
      volume_val =
        self->attack_level * ((gdouble) ts /
                              (gdouble) self->attack_duration_time);
      break;

    case decay:
      /* When the attack is complete we decay to the specified sustain
       * level, reaching it at the specified decay duration time following 
       * completion of the attack.  Do a linear interpolation between the 
       * attack level and the sustain level.  If the decay duration time is 0, 
       * we will never be in the decay section of the envelope.  */
      decay_end_time = self->attack_duration_time + self->decay_duration_time;
      decay_fraction =
        (gdouble) 1.0 - (gdouble) (decay_end_time -
                                   ts) / (gdouble) self->decay_duration_time;
      GST_LOG_OBJECT (self, "decay, fraction %g.", decay_fraction);
      volume_val =
        (decay_fraction * self->sustain_level) +
        ((1.0 - decay_fraction) * self->attack_level);
      break;

    case sustain:
      GST_LOG_OBJECT (self, "sustain");
      /* When the decay is complete we stay at the sustain level until release.
       */
      volume_val = self->sustain_level;
      break;

    case release:
      /* Upon release we reduce the volume to 0 over the release duration time.
       * However, if the release duration time is infinite, we do not
       * reduce the volume.  Normally, release will occur after sustain,
       * so an infinite release duration time will keep the volume at the
       * sustain level.  If the release duration time is 0 we do not get to
       * the release portion of the envelope but go directly to completed.  */
      if (self->release_duration_infinite)
        {
          volume_val = self->release_start_volume;
          GST_LOG_OBJECT (self, "release, infinite.");
          break;
        }

      release_fraction =
        (gdouble) (ts -
                   self->release_start_time) /
        (gdouble) self->release_duration_time;
      GST_LOG_OBJECT (self, "release, fraction is %g.", release_fraction);
      volume_val = self->release_start_volume * (1.0 - release_fraction);
      break;

    case completed:
      GST_LOG_OBJECT (self, "completed");
      /* We are beyond the release duration; volume is always 0.  */
      volume_val = 0.0;
      break;
    }

  /* Remember the last value used, so we can release from it in case the
   * release starts at an unusual time in the envelope.  */
  self->last_volume = volume_val;

  /* Allow for scaling the envelope, perhaps from a Note On velocity.  */
  volume_val = volume_val * self->volume;

  GST_LOG_OBJECT (self,
                  "at time %" GST_TIME_FORMAT ", envelope volume is %g.",
                  GST_TIME_ARGS (ts), volume_val);

  return volume_val;
}

static gboolean
envelope_stop (GstBaseTransform * base)
{
  /* GstEnvelope *self = GST_ENVELOPE (base); */
  return GST_CALL_PARENT_WITH_DEFAULT (GST_BASE_TRANSFORM_CLASS, stop, (base),
                                       TRUE);
};

/* call whenever the format changes.  */
static gboolean
envelope_setup (GstAudioFilter * filter, const GstAudioInfo * info)
{
  GstEnvelope *self = GST_ENVELOPE (filter);
  GST_OBJECT_LOCK (self);
  GST_OBJECT_UNLOCK (self);
  return TRUE;
};

static void
gst_envelope_dispose (GObject * object)
{
  GstEnvelope *self = GST_ENVELOPE (object);
  g_free (self->release_duration_string);
  self->release_duration_string = NULL;
  G_OBJECT_CLASS (parent_class)->dispose (object);
};

/* GObject vmethod implementations */

/* initialize the envelope's class */
static void
gst_envelope_class_init (GstEnvelopeClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstBaseTransformClass *trans_class;
  GstAudioFilterClass *filter_class;
  GstCaps *caps;
  GParamSpec *param_spec;
  gchar *release_duration_default;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  trans_class = (GstBaseTransformClass *) klass;
  filter_class = (GstAudioFilterClass *) (klass);

  gobject_class->set_property = gst_envelope_set_property;
  gobject_class->get_property = gst_envelope_get_property;
  gobject_class->dispose = gst_envelope_dispose;

  param_spec =
    g_param_spec_boolean ("silent", "Silent", "Produce verbose output ?",
                          FALSE, G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE);
  g_object_class_install_property (gobject_class, PROP_SILENT, param_spec);

  param_spec =
    g_param_spec_uint64 ("attack-duration-time", "Attack_duration_time",
                         "Time for initial ramp up of volume", 0, G_MAXUINT64,
                         0, G_PARAM_READWRITE);
  g_object_class_install_property (gobject_class, PROP_ATTACK_DURATION_TIME,
                                   param_spec);

  param_spec =
    g_param_spec_double ("attack-level", "Attack_level",
                         "Volume level to reach at end of attack", 0, 10.0,
                         1.0, G_PARAM_READWRITE);
  g_object_class_install_property (gobject_class, PROP_ATTACK_LEVEL,
                                   param_spec);

  param_spec =
    g_param_spec_uint64 ("decay-duration-time", "Decay_duration_time",
                         "Time for ramp down to sustain level after attack",
                         0, G_MAXUINT64, 0, G_PARAM_READWRITE);
  g_object_class_install_property (gobject_class, PROP_DECAY_DURATION_TIME,
                                   param_spec);

  param_spec =
    g_param_spec_double ("sustain-level", "Sustain_level",
                         "Volume level to reach at end of decay", 0, 10.0,
                         1.0, G_PARAM_READWRITE);
  g_object_class_install_property (gobject_class, PROP_SUSTAIN_LEVEL,
                                   param_spec);
  param_spec =
    g_param_spec_uint64 ("release-start-time", "Release_start_time",
                         "When to start the release process", 0, G_MAXUINT64,
                         0, G_PARAM_READWRITE);
  g_object_class_install_property (gobject_class, PROP_RELEASE_START_TIME,
                                   param_spec);

  release_duration_default = g_strdup ((gchar *) "0");
  param_spec =
    g_param_spec_string ("release-duration-time", "Release_duration_time",
                         "Time for ramp down to 0 while releasing, may be ∞",
                         release_duration_default, G_PARAM_READWRITE);
  g_object_class_install_property (gobject_class, PROP_RELEASE_DURATION_TIME,
                                   param_spec);
  g_free (release_duration_default);
  release_duration_default = NULL;

  param_spec =
    g_param_spec_double ("volume", "Volume_level", "Volume to scale envelope",
                         0, 10.0, 1.0, G_PARAM_READWRITE);
  g_object_class_install_property (gobject_class, PROP_VOLUME, param_spec);

  gst_element_class_set_static_metadata (gstelement_class, "Envelope",
                                         "Filter/Effect/Audio",
                                         "Shape the sound using "
                                         "an a-d-s-r envelope",
                                         "John Sauter <John_Sauter@"
                                         "systemeyescomputerstore.com>");

  caps = gst_caps_from_string (ALLOWED_CAPS);
  gst_audio_filter_class_add_pad_templates (filter_class, caps);
  gst_caps_unref (caps);

  trans_class->before_transform =
    GST_DEBUG_FUNCPTR (envelope_before_transform);
  trans_class->transform_ip = GST_DEBUG_FUNCPTR (envelope_transform_ip);
  trans_class->transform = GST_DEBUG_FUNCPTR (envelope_transform);
  trans_class->stop = GST_DEBUG_FUNCPTR (envelope_stop);
  trans_class->transform_ip_on_passthrough = FALSE;

  filter_class->setup = envelope_setup;
}

/* initialize the new element
 * initialize instance structure
 */
static void
gst_envelope_init (GstEnvelope * self)
{
  /* Set all of the parameters to their default values.  */
  self->silent = FALSE;
  self->attack_duration_time = 0;
  self->attack_level = 1.0;
  self->decay_duration_time = 0;
  self->sustain_level = 1.0;
  self->release_start_time = 0;
  self->release_duration_string = g_strdup ((gchar *) "0");
  self->release_duration_time = 0;
  self->release_duration_infinite = FALSE;
  self->release_start_volume = 0.0;
  self->release_triggered = FALSE;
  self->volume = 1.0;
}

/* Set a property.  */
static void
gst_envelope_set_property (GObject * object, guint prop_id,
                           const GValue * value, GParamSpec * pspec)
{
  GstEnvelope *self = GST_ENVELOPE (object);

  switch (prop_id)
    {
    case PROP_SILENT:
      GST_OBJECT_LOCK (self);
      self->silent = g_value_get_boolean (value);
      GST_INFO_OBJECT (self, "silent set to %d.", self->silent);
      GST_OBJECT_UNLOCK (self);
      break;

    case PROP_ATTACK_DURATION_TIME:
      GST_OBJECT_LOCK (self);
      self->attack_duration_time = g_value_get_uint64 (value);
      GST_INFO_OBJECT (self,
                       "attack-duration-time set to %" G_GUINT64_FORMAT ".",
                       self->attack_duration_time);
      GST_OBJECT_UNLOCK (self);
      break;

    case PROP_ATTACK_LEVEL:
      GST_OBJECT_LOCK (self);
      self->attack_level = g_value_get_double (value);
      GST_INFO_OBJECT (self, "attack-level set to %g.", self->attack_level);
      GST_OBJECT_UNLOCK (self);
      break;

    case PROP_DECAY_DURATION_TIME:
      GST_OBJECT_LOCK (self);
      self->decay_duration_time = g_value_get_uint64 (value);
      GST_INFO_OBJECT (self,
                       "decay-duration-time set to %" G_GUINT64_FORMAT ".",
                       self->decay_duration_time);
      GST_OBJECT_UNLOCK (self);
      break;

    case PROP_SUSTAIN_LEVEL:
      GST_OBJECT_LOCK (self);
      self->sustain_level = g_value_get_double (value);
      GST_INFO_OBJECT (self, "sustain-level set to %g.", self->sustain_level);
      GST_OBJECT_UNLOCK (self);
      break;

    case PROP_RELEASE_START_TIME:
      GST_OBJECT_LOCK (self);
      self->release_start_time = g_value_get_uint64 (value);
      GST_INFO_OBJECT (self,
                       "release-start-time set to %" G_GUINT64_FORMAT ".",
                       self->release_start_time);
      GST_OBJECT_UNLOCK (self);
      break;

    case PROP_RELEASE_DURATION_TIME:
      GST_OBJECT_LOCK (self);
      g_free (self->release_duration_string);
      self->release_duration_string = NULL;
      self->release_duration_string = g_value_dup_string (value);

      if (g_strcmp0 (self->release_duration_string, "∞") == 0)
        {
          self->release_duration_infinite = TRUE;
          self->release_duration_time = 0;
        }
      else
        {
          self->release_duration_infinite = FALSE;
          self->release_duration_time =
            g_ascii_strtoull (self->release_duration_string, NULL, 10);
        }
      GST_INFO_OBJECT (self, "release-duration-time set to %s.",
                       self->release_duration_string);
      GST_OBJECT_UNLOCK (self);
      break;

    case PROP_VOLUME:
      GST_OBJECT_LOCK (self);
      self->volume = g_value_get_double (value);
      GST_INFO_OBJECT (self, "volume set to %g.", self->volume);
      GST_OBJECT_UNLOCK (self);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gst_envelope_get_property (GObject * object, guint prop_id, GValue * value,
                           GParamSpec * pspec)
{
  GstEnvelope *self = GST_ENVELOPE (object);

  switch (prop_id)
    {
    case PROP_SILENT:
      GST_OBJECT_LOCK (self);
      g_value_set_boolean (value, self->silent);
      GST_OBJECT_UNLOCK (self);
      break;

    case PROP_ATTACK_DURATION_TIME:
      GST_OBJECT_LOCK (self);
      g_value_set_uint64 (value, self->attack_duration_time);
      GST_OBJECT_UNLOCK (self);
      break;

    case PROP_ATTACK_LEVEL:
      GST_OBJECT_LOCK (self);
      g_value_set_double (value, self->attack_level);
      GST_OBJECT_UNLOCK (self);
      break;

    case PROP_DECAY_DURATION_TIME:
      GST_OBJECT_LOCK (self);
      g_value_set_uint64 (value, self->decay_duration_time);
      GST_OBJECT_UNLOCK (self);
      break;

    case PROP_SUSTAIN_LEVEL:
      GST_OBJECT_LOCK (self);
      g_value_set_double (value, self->sustain_level);
      GST_OBJECT_UNLOCK (self);
      break;

    case PROP_RELEASE_START_TIME:
      GST_OBJECT_LOCK (self);
      g_value_set_uint64 (value, self->release_start_time);
      GST_OBJECT_UNLOCK (self);
      break;

    case PROP_RELEASE_DURATION_TIME:
      GST_OBJECT_LOCK (self);
      g_value_set_string (value, self->release_duration_string);
      GST_OBJECT_UNLOCK (self);
      break;

    case PROP_VOLUME:
      GST_OBJECT_LOCK (self);
      g_value_set_double (value, self->volume);
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
envelope_init (GstPlugin * envelope)
{
  return gst_element_register (envelope, "envelope", GST_RANK_NONE,
                               GST_TYPE_ENVELOPE);
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR, GST_VERSION_MINOR, envelope,
                   "Shape the sound using an a-d-s-r envelope", envelope_init,
                   VERSION, "LGPL", "GStreamer", "http://gstreamer.net/")
