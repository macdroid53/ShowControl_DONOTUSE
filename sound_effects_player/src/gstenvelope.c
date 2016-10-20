/*
 * File: gstenvelope.c, part of Show_control, a Gstreamer application
 *
 * Much of this code is based on Gstreamer examples and tutorials.
 *
 * Copyright © 2016 John Sauter <John_Sauter@systemeyescomputerstore.com>
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
 * U+221E, which is ∞.  Default is 0, which causes the sound to be shut
 * down instantly.
 *
 * #GstEnvelope:volume is the normal volume of the sound; the envelope
 * is scaled by this amount.  It might be used to implement the note-on
 * velocity from a musical instrument.  Default is 1.0.
 *
 * #GstEnvelope:autostart defaults to FALSE.  If you set it to TRUE,
 * envelope processing will start as soon as sound data passes through
 * it, without waiting for a Start event.
 *
 * #GstEnvelope:sound-name is the name of the sound being shaped.  This name
 * is used in messages to the application, to identify the sound.  It
 * defaults to the empty string.
 *
 * If all the properties except autostart are defaulted, and release is never 
 * signaled, this audio filter does not change the sound passing through it.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch-1.0 -v -m audiotestsrc ! envelope attack-duration-time=1000000000 autostart=TRUE ! fakesink silent=TRUE
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
  PROP_VOLUME,
  PROP_AUTOSTART,
  PROP_SOUND_NAME
};

/* For simplicity, we handle only floating point samples.
 * If there is a need for conversion to another type, it can be
 * done using an audioconvert element.  */

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
			   "Shape the amplitude of the sound");
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

static gboolean envelope_sink_event_handler (GstBaseTransform * trans,
                                             GstEvent * event);
static gboolean envelope_src_event_handler (GstBaseTransform * trans,
                                            GstEvent * event);

static gdouble compute_volume (GstEnvelope * self, GstClockTime timestamp);

/* Before each transform of input to output, do this.  */
static void
envelope_before_transform (GstBaseTransform * base, GstBuffer * buffer)
{
  GstClockTime timestamp, duration;
  GstEnvelope *self = GST_ENVELOPE (base);
  GstStructure *structure;
  GstMessage *message;
  gboolean result;
  GValue sound_name_value = G_VALUE_INIT;

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

  /* If we have reached the release portion of the envelope, tell the
   * application.  */
  if (self->running && self->release_started
      && !self->application_notified_release)
    {
      GST_INFO_OBJECT (self, "sound has entered its Release stage");
      structure = gst_structure_new_empty ("release_started");
      g_value_init (&sound_name_value, G_TYPE_STRING);
      g_value_set_string (&sound_name_value, self->sound_name);
      gst_structure_set_value (structure, (gchar *) "sound_name",
                               &sound_name_value);

      message = gst_message_new_element (GST_OBJECT (self), structure);
      result = gst_element_post_message (GST_ELEMENT (self), message);
      if (!result)
        {
          GST_DEBUG_OBJECT (self, "unable to post a release_started message");
        }
      self->application_notified_release = TRUE;
      g_value_unset (&sound_name_value);
    }

  /* If we have completed the sound, tell the application.  */
  if (self->completed && !self->application_notified_completion)
    {
      GST_INFO_OBJECT (self, "sound has completed");
      structure = gst_structure_new_empty ("completed");
      g_value_init (&sound_name_value, G_TYPE_STRING);
      g_value_set_string (&sound_name_value, self->sound_name);
      gst_structure_set_value (structure, (gchar *) "sound_name",
                               &sound_name_value);

      message = gst_message_new_element (GST_OBJECT (self), structure);
      result = gst_element_post_message (GST_ELEMENT (self), message);
      if (!result)
        {
          GST_DEBUG_OBJECT (self, "unable to post a completed message");
        }
      self->application_notified_completion = TRUE;
      g_value_unset (&sound_name_value);
    }


  /* If we have completed the envelope, including the release stage,
   * and we are not autostarting, recycle it so we can use it again.  
   * Note that this test is performed before the start test, so a Start
   * message that arrives during the release stage of the envelope will
   * restart it immediately after the release is complete.  */
  if (self->completed && !self->autostart)
    {
      GST_DEBUG_OBJECT (self,
                        "recycling envelope, base time is %" GST_TIME_FORMAT
                        ".", GST_TIME_ARGS (self->base_time));
      self->running = FALSE;
      self->completed = FALSE;
      self->release_started = FALSE;
      self->base_time = 0;
      self->last_volume = 0;
      self->application_notified_release = FALSE;
      self->application_notified_completion = FALSE;
    }

  if (self->running)
    {
      GST_DEBUG_OBJECT (self, "running, base time is %" GST_TIME_FORMAT ".",
                        GST_TIME_ARGS (self->base_time));
      GST_DEBUG_OBJECT (self, "envelope time is %" GST_TIME_FORMAT ".",
                        GST_TIME_ARGS (timestamp - self->base_time));
    }

  /* If we have seen a start message, or if we are autostarted,
   * and the envelope is not yet running, start running it.  */
  if ((!self->running) && (self->started || self->autostart))
    {
      self->external_release_seen = FALSE;
      self->external_completion_seen = FALSE;
      self->running = TRUE;
      self->started = FALSE;
      self->pause_seen = FALSE;
      self->continue_seen = FALSE;
      self->pausing = FALSE;
      self->base_time = timestamp;
      self->pause_time = 0;
      GST_DEBUG_OBJECT (self,
                        "starting envelope, base time set to %"
                        GST_TIME_FORMAT ".", GST_TIME_ARGS (self->base_time));
    }

  return;
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
  GstClockTimeDiff interval = gst_util_uint64_scale_int (1, GST_SECOND, rate);
  GstClockTimeDiff pause_duration;

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

  /* Handle pause and continue events.  */
  if ((self->pause_seen) && (!self->pausing))
    {
      /* We are starting a pause.  Record the current time so we can
       * determine the pause duration when the pause ends.  */
      self->pause_start_time = ts;
      self->pausing = TRUE;
      GST_DEBUG_OBJECT (self, "pause starts at %" GST_TIME_FORMAT ".",
                        GST_TIME_ARGS (ts));
    }
  if ((self->pause_seen) && (self->continue_seen))
    {
      /* This is the end of the pause.  */
      self->pause_seen = FALSE;
      self->pausing = FALSE;
      self->continue_seen = FALSE;
      /* Accumulate the time spend pausing, so the envelope can continue
       * through its progression.  */
      pause_duration = ts - self->pause_start_time;
      self->pause_time = self->pause_time + pause_duration;
      GST_DEBUG_OBJECT (self,
                        "pause is completed, duration: %" GST_TIME_FORMAT ".",
                        GST_TIME_ARGS (pause_duration));
    }
  GST_DEBUG_OBJECT (self, "pause time is: %" GST_TIME_FORMAT ".",
                    GST_TIME_ARGS (self->pause_time));

  if (self->running)
    {
      GST_DEBUG_OBJECT (self, "envelope time: %" GST_TIME_FORMAT ".",
                        GST_TIME_ARGS (ts - self->base_time));
    }
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
      /* Compute the volume at this time step.  */
      volume_val =
        compute_volume (self, ts - self->base_time - self->pause_time);

      /* Apply that volume to each channel.  */
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
  GstClockTimeDiff interval = gst_util_uint64_scale_int (1, GST_SECOND, rate);
  GstClockTimeDiff pause_duration;

  /* Get the number of frames to process.  Each frame has a sample for
   * each channel, and each sample contains "width" bits.  */
  frame_count = gst_buffer_get_size (inbuf) / (width * channel_count / 8);

  ts = GST_BUFFER_TIMESTAMP (outbuf);
  ts = gst_segment_to_stream_time (&base->segment, GST_FORMAT_TIME, ts);
  GST_DEBUG_OBJECT (self, "transform timestamp: %" GST_TIME_FORMAT ".",
                    GST_TIME_ARGS (ts));
  /* Handle pause and continue events.  */
  if ((self->pause_seen) && (!self->pausing))
    {
      /* We are starting a pause.  Record the current time so we can
       * determine the pause duration when the pause ends.  */
      self->pause_start_time = ts;
      self->pausing = TRUE;
      GST_DEBUG_OBJECT (self, "pause starts at %" GST_TIME_FORMAT ".",
                        GST_TIME_ARGS (ts));
    }
  if ((self->pause_seen) && (self->continue_seen))
    {
      /* This is the end of the pause.  */
      self->pause_seen = FALSE;
      self->pausing = FALSE;
      self->continue_seen = FALSE;
      /* Accumulate the time spend pausing, so the envelope can continue
       * through its progression.  */
      pause_duration = ts - self->pause_start_time;
      self->pause_time = self->pause_time + pause_duration;
      GST_DEBUG_OBJECT (self,
                        "pause is completed, duration: %" GST_TIME_FORMAT ".",
                        GST_TIME_ARGS (pause_duration));
    }
  GST_DEBUG_OBJECT (self, "pause time is: %" GST_TIME_FORMAT ".",
                    GST_TIME_ARGS (self->pause_time));
  if (self->running)
    {
      GST_DEBUG_OBJECT (self, "envelope time: %" GST_TIME_FORMAT ".",
                        GST_TIME_ARGS (ts - self->base_time));
    }
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

      /* Compute the volume at this time step.  */
      volume_val =
        compute_volume (self, ts - self->base_time - self->pause_time);

      /* Apply that volume to each channel.  */
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

/* This enumeration type indicates a stage of envelope processing.  */
enum envelope_stage
{ not_started, attack, decay, sustain, release, completed, pausing };

/* Determine the stage of envelope processing, given the time since
 * the envelope started, minus the time spent paused.  */
static enum envelope_stage
compute_envelope_stage (GstEnvelope * self, GstClockTime ts)
{
  gchar *release_type;

  /* Decide where we are in the amplitude envelope.  The normal progression
   * after the note has started is attack, decay, sustain, release, completed.
   * However, a release event can arrive at at any time.  If the decay 
   * duration time is 0 we go straight from attack to sustain, so sustain 
   * level should equal attack level in this case.  Also, a pause event
   * can arrive at any time, and will delay the envelope progression.  */

  /* if the envelope is not running, it is not yet started.  */
  if (!self->running)
    {
      return not_started;
    }

  if (self->pausing)
    {
      return pausing;
    }

  if (self->external_release_seen || self->external_completion_seen)
    {
      /* We have seen an external signal initiating the release process,
       * so the envelope is in either its release or completed stage.  */

      /* If this is the first time we have been in the release stage,
       * remember the time and volume since we will need them to ramp the
       * volume down to zero.  */
      if (!self->release_started)
        {
          self->release_started = TRUE;
          self->release_started_volume = self->last_volume;
          self->release_started_time = ts;
          release_type = (gchar *) "an unknown";
          if (self->external_completion_seen)
            {
              release_type = (gchar *) "a complete";
            }
          if (self->external_release_seen)
            {
              release_type = (gchar *) "a release";
            }
          GST_INFO_OBJECT (self,
                           "Release triggered by %s event at %"
                           GST_TIME_FORMAT " with volume %f.", release_type,
                           GST_TIME_ARGS (self->release_started_time),
                           self->release_started_volume);
        }
      /* An external completion message means we have reached the end of
       * the sound from upstream.  No matter where we were in the envelope,
       * we are now done.  */
      if (self->external_completion_seen)
        return completed;

      /* Otherwise, if we are within the release duration, we are in the 
       * release stage of the envelope.  Note that if the release duration
       * is infinite, the volume stays at the value it held when the
       * release message arrived until the sound coming from upstream
       * is complete.
       */
      if ((ts < (self->release_started_time + self->release_duration_time))
          || (self->release_duration_infinite))
        return release;
      else
        return completed;
    }

  /* We have not seen a release or completion event, so the envelope proceeds
   * along its normal path.  */
  if (ts < self->attack_duration_time)
    {
      /* The attack is not yet complete.  */
      return attack;
    }

  /* The attack is complete.  */

  if (ts < self->attack_duration_time + self->decay_duration_time)
    {
      /* The decay is not yet complete.  */
      return decay;
    }

  /* The decay is complete.  */

  if ((ts < self->release_start_time) || (self->release_start_time == 0))
    {
      /* The decay is complete but we have not yet started 
       * release.  */
      return sustain;
    }

  /* A non-infinite, non-zero release time was specified for the envelope.
   */

  if (self->release_duration_infinite
      || ts < (self->release_start_time + self->release_duration_time))
    {
      /* The release section of the envelope is running.
       * If this is the first time we have been in the release stage,
       * remember the time and volume since we will need them to ramp the
       * volume down to zero.  */
      if (!self->release_started)
        {
          self->release_started = TRUE;
          self->release_started_volume = self->last_volume;
          self->release_started_time = ts;
          GST_INFO_OBJECT (self,
                           "Release triggered at %" GST_TIME_FORMAT
                           " with volume %f.",
                           GST_TIME_ARGS (self->release_started_time),
                           self->release_started_volume);
        }
      return release;
    }

  /* We have passed the specified release time, and in addition the specified
   * release duration.  The envelope is complete.  */
  return completed;
}

/* Compute the volume adjustment for a frame.  */
static gdouble
compute_volume (GstEnvelope * self, GstClockTime ts)
{
  gdouble volume_val;
  enum envelope_stage envelope_position;
  GstClockTime decay_end_time;
  gdouble attack_fraction, decay_fraction, release_fraction;

  /* Decide where we are in the amplitude envelope.  */
  envelope_position = compute_envelope_stage (self, ts);

  switch (envelope_position)
    {
    case pausing:
      /* The note is paused.  */
      GST_LOG_OBJECT (self, "envelope position: pausing");
      volume_val = 0.0;
      break;

    case not_started:
      /* If the note has not yet started, the volume is 0.  */
      GST_LOG_OBJECT (self, "envelope position: not started");
      volume_val = 0.0;
      break;

    case attack:
      /* The initial attack.  We ramp up to the specified attack level,
       * reaching it at the specified attack duration time.  */
      attack_fraction = (gdouble) ts / (gdouble) self->attack_duration_time;

      GST_LOG_OBJECT (self, "envelope position: attack, fraction %g.",
                      attack_fraction);
      volume_val = self->attack_level * attack_fraction;
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
      GST_LOG_OBJECT (self, "envelope position: decay, fraction %g.",
                      decay_fraction);
      volume_val =
        (decay_fraction * self->sustain_level) +
        ((1.0 - decay_fraction) * self->attack_level);
      break;

    case sustain:
      GST_LOG_OBJECT (self, "envelope position: sustain");
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
          volume_val = self->release_started_volume;
          GST_LOG_OBJECT (self, "envelope position: release, infinite.");
          break;
        }

      release_fraction =
        (gdouble) (ts -
                   self->release_started_time) /
        (gdouble) self->release_duration_time;
      GST_LOG_OBJECT (self, "envelope position: release, fraction %g.",
                      release_fraction);
      volume_val = self->release_started_volume * (1.0 - release_fraction);

      break;

    case completed:
      GST_LOG_OBJECT (self, "envelope position: completed");
      /* We are beyond the release duration; volume is always 0.  */
      volume_val = 0.0;
      /* Note the envelope completion.  This is used to recycle the envelope.
       */
      if (!self->completed)
        {
          GST_DEBUG_OBJECT (self,
                            "envelope completed at envelope time %"
                            GST_TIME_FORMAT ".", GST_TIME_ARGS (ts));
        }

      self->completed = TRUE;
      break;
    }

  /* Remember the last value used, so we can release from it in case the
   * release starts at an unusual time in the envelope.  */
  self->last_volume = volume_val;

  /* Allow for scaling the envelope, perhaps to implement a Note On velocity.  
   */
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
  g_free (self->last_message);
  self->last_message = NULL;
  g_free (self->sound_name);
  self->sound_name = NULL;
  G_OBJECT_CLASS (parent_class)->dispose (object);
};

/* GObject vmethod implementations */

/* initialize the envelope's class */
static void
gst_envelope_class_init (GstEnvelopeClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *element_class;
  GstBaseTransformClass *trans_class;
  GstAudioFilterClass *filter_class;
  GstCaps *caps;
  GParamSpec *param_spec;
  gchar *release_duration_default;
  gchar *sound_name_default;

  gobject_class = (GObjectClass *) klass;
  element_class = (GstElementClass *) klass;
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

  param_spec =
    g_param_spec_boolean ("autostart", "Autostart",
                          "do not wait for a Start event", FALSE,
                          G_PARAM_READWRITE);
  g_object_class_install_property (gobject_class, PROP_AUTOSTART, param_spec);

  sound_name_default = g_strdup ("");
  param_spec =
    g_param_spec_string ("sound-name", "Sound_name",
                         "The name of the sound being shaped",
                         sound_name_default, G_PARAM_READWRITE);
  g_object_class_install_property (gobject_class, PROP_SOUND_NAME,
                                   param_spec);
  g_free (sound_name_default);
  sound_name_default = NULL;

  gst_element_class_set_static_metadata (element_class, "Envelope",
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
  trans_class->sink_event = GST_DEBUG_FUNCPTR (envelope_sink_event_handler);
  trans_class->src_event = GST_DEBUG_FUNCPTR (envelope_src_event_handler);

  filter_class->setup = envelope_setup;
}

/* initialize the new element
 * initialize instance structure
 */
static void
gst_envelope_init (GstEnvelope * self)
{
  /* Set all of the parameters to their default values and initialize
   * the locals.  */
  self->silent = FALSE;
  self->attack_duration_time = 0;
  self->attack_level = 1.0;
  self->decay_duration_time = 0;
  self->sustain_level = 1.0;
  self->release_start_time = 0;
  self->release_duration_string = g_strdup ((gchar *) "0");
  self->release_duration_time = 0;
  self->release_duration_infinite = FALSE;
  self->release_started_volume = 0.0;
  self->release_started_time = 0;
  self->release_started = FALSE;
  self->volume = 1.0;
  self->autostart = FALSE;
  self->sound_name = g_strdup ("");

  self->external_release_seen = FALSE;
  self->external_completion_seen = FALSE;
  self->running = FALSE;
  self->started = FALSE;
  self->completed = FALSE;
  self->pause_seen = FALSE;
  self->continue_seen = FALSE;
  self->pausing = FALSE;
  self->last_message = NULL;
  self->application_notified_release = FALSE;
  self->application_notified_completion = FALSE;
  self->base_time = 0;
  self->pause_time = 0;
  self->pause_start_time = 0;
  self->last_volume = 0;
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

    case PROP_AUTOSTART:
      GST_OBJECT_LOCK (self);
      self->autostart = g_value_get_boolean (value);
      GST_INFO_OBJECT (self, "autostart set to %d.", self->autostart);
      GST_OBJECT_UNLOCK (self);
      break;

    case PROP_SOUND_NAME:
      GST_OBJECT_LOCK (self);
      g_free (self->sound_name);
      self->sound_name = g_value_dup_string (value);
      GST_INFO_OBJECT (self, "sound-name set to %s.", self->sound_name);
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

    case PROP_AUTOSTART:
      GST_OBJECT_LOCK (self);
      g_value_set_boolean (value, self->autostart);
      GST_OBJECT_UNLOCK (self);
      break;

    case PROP_SOUND_NAME:
      GST_OBJECT_LOCK (self);
      g_value_set_string (value, self->sound_name);
      GST_OBJECT_UNLOCK (self);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/* This event handler is called when an event is sent to the source pad.
 * We care only about custom events: start, pause, continue and release.
 */
static gboolean
envelope_src_event_handler (GstBaseTransform * trans, GstEvent * event)
{
  GstEnvelope *self = GST_ENVELOPE (trans);
  const GstStructure *event_structure;
  const gchar *structure_name;
  const gchar *event_name;
  gchar *structure_as_string;
  gboolean ret;

  GST_OBJECT_LOCK (self);
  g_free (self->last_message);
  self->last_message = NULL;
  event_name = gst_event_type_get_name (GST_EVENT_TYPE (event));
  event_structure = gst_event_get_structure (event);
  if (event_structure != NULL)
    {
      structure_as_string = gst_structure_to_string (event_structure);
    }
  else
    {
      structure_as_string = g_strdup ("");
    }
  self->last_message =
    g_strdup_printf ("src event (%s:%s) type: %s (%d), %s %p",
                     GST_DEBUG_PAD_NAME (trans->sinkpad), event_name,
                     GST_EVENT_TYPE (event), structure_as_string, event);
  g_free (structure_as_string);
  GST_INFO_OBJECT (self, "%s", self->last_message);
  GST_OBJECT_UNLOCK (self);

  if (event_structure != NULL)
    {
      structure_name = gst_structure_get_name (event_structure);
    }
  else
    {
      structure_name = (gchar *) "";
    }

  switch (GST_EVENT_TYPE (event))
    {
    case GST_EVENT_CUSTOM_UPSTREAM:
      if (g_strcmp0 (structure_name, (gchar *) "release") == 0)
        {
          /* This is a release event, which might be caused by receipt
           * of a Note Off MIDI message, or by an operator pushing a
           * stop button.  Set a flag that will force release processing
           * to begin.  */
          GST_INFO_OBJECT (self, "Received custom release event");
          GST_OBJECT_LOCK (self);
          self->external_release_seen = TRUE;
          GST_OBJECT_UNLOCK (self);
        }
      if (g_strcmp0 (structure_name, (gchar *) "start") == 0)
        {
          /* This is a start event, which might be caused by receipt
           * of a Note On MIDI message, or by an operator pushing a
           * start button.  Flag that we have seen the message; the
           * next incoming buffer will start the envelope running
           * as soon as the previous release is complete.  */
          GST_INFO_OBJECT (self, "Received custom start event");
          GST_OBJECT_LOCK (self);
          self->started = TRUE;
          GST_OBJECT_UNLOCK (self);
        }
      if (g_strcmp0 (structure_name, (gchar *) "pause") == 0)
        {
          /* This is a pause event, caused by an operator pushing the
           * pause button.  Flag that we have seen the message; we will not
           * advance through the envelope until we see a continue event.  */
          GST_INFO_OBJECT (self, "Received custom pause event");
          GST_OBJECT_LOCK (self);
          self->pause_seen = TRUE;
          GST_OBJECT_UNLOCK (self);
        }
      if (g_strcmp0 (structure_name, (gchar *) "continue") == 0)
        {
          /* This is a continue event, caused by an operator pushing the
           * continue button to cancel a previous pause.  Flag that we
           * have seen the message.  We don't simply clear the pause flag
           * because we want to notice the transition.  */
          GST_INFO_OBJECT (self, "Received custom continue event");
          GST_OBJECT_LOCK (self);
          self->continue_seen = TRUE;
          GST_OBJECT_UNLOCK (self);
        }
      break;

    case GST_EVENT_EOS:
      /* We have reached the end of the incoming data stream.  
       * Set a flag that will cause the sound to stop.  */
      GST_DEBUG_OBJECT (self, "envelope completion EOS");
      GST_OBJECT_LOCK (self);
      self->external_completion_seen = TRUE;
      GST_OBJECT_UNLOCK (self);
      break;

    default:
      break;
    }

  /* When we are done with the event, do the default processing on it.  */
  ret = GST_BASE_TRANSFORM_CLASS (parent_class)->src_event (trans, event);

  return ret;
}

/* This event handler is called when an event is sent to the sink pad.
 * The event we care about here is the completion event, which is sent
 * by the looper when it reaches the end of its buffer.
 */
static gboolean
envelope_sink_event_handler (GstBaseTransform * trans, GstEvent * event)
{
  GstEnvelope *self = GST_ENVELOPE (trans);
  const GstStructure *event_structure;
  const gchar *event_name;
  const gchar *structure_name;
  gchar *structure_as_string;
  gboolean ret;

  GST_OBJECT_LOCK (self);
  g_free (self->last_message);
  self->last_message = NULL;
  event_name = gst_event_type_get_name (GST_EVENT_TYPE (event));
  event_structure = gst_event_get_structure (event);
  if (event_structure != NULL)
    {
      structure_as_string = gst_structure_to_string (event_structure);
    }
  else
    {
      structure_as_string = g_strdup ("");
    }
  self->last_message =
    g_strdup_printf ("sink event (%s:%s) type: %s (%d), %s %p",
                     GST_DEBUG_PAD_NAME (trans->sinkpad), event_name,
                     GST_EVENT_TYPE (event), structure_as_string, event);
  g_free (structure_as_string);
  GST_INFO_OBJECT (self, "%s", self->last_message);
  GST_OBJECT_UNLOCK (self);

  if (event_structure != NULL)
    {
      structure_name = gst_structure_get_name (event_structure);
    }
  else
    {
      structure_name = (gchar *) "";
    }

  switch (GST_EVENT_TYPE (event))
    {
    case GST_EVENT_CUSTOM_DOWNSTREAM:
      GST_INFO_OBJECT (self, "Processing %s.", structure_name);
      if (g_strcmp0 (structure_name, (gchar *) "complete") == 0)
        {
          /* This is a complete event, which is sent by the looper when
           * it reaches the end of its buffer.  Set a flag that will
           * cause the sound to stop.  */
          GST_DEBUG_OBJECT (self, "envelope completion message");
          GST_OBJECT_LOCK (self);
          self->external_completion_seen = TRUE;
          GST_OBJECT_UNLOCK (self);
        }
      break;

    default:
      break;
    }

  /* When we are done with the event, do the default processing on it.  */
  ret = GST_BASE_TRANSFORM_CLASS (parent_class)->sink_event (trans, event);

  return ret;
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
