/*
 * gstlooper.c, a file in sound_effects_player, a component of Show_control, 
 * which is a Gstreamer application.  Much of the code in this file is based
 * on Gstreamer examples and tutorials.
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
 * SECTION:element-looper
 * @short_description: Repeat a section of the input stream.
 *
 * This element places its input into a buffer, then sends it downstream
 * a specified number of times.  Parameters control the number of times
 * the data is sent, and can specify a start and end point within the data.
 * Messages are used to start and stop the element, and to pause it.
 *
 * Properties are:
 *
 * #GstLooper:loop-to is the beginning of the section to repeat, in nanoseconds 
 * from the beginning of the input.  Default is 0.  
 *
 * #GstLooper:loop-from is the end of the section to repeat, also in 
 * nanoseconds.  If the sample rate is less than 1,000,000,000
 * samples per second, looping at exactly loop-from and loop-to might not
 * be possible, in which case the loop starts at the beginning of the sample
 * at loop-to and ends after the sample at loop-from.  Default is 0, which
 * suppresses looping.
 *
 * #GstLooper:loop-limit is the number of times to repeat the loop; 0 means 
 * repeat indefinitely, which is the default.  
 *
 * #GstLooper:max-duration-time, if specified, is the maximum amount of time 
 * from the source to be held for repeating, in nanoseconds.  
 * This can be useful with live or infinite sources.  Note that this element 
 * can itself be an infinite source for its downstream consumers, even if its 
 * upstream is finite or limited by max-duration.  If max-duration is not 
 * specified, the looper element will attempt to absorb all sound provided to 
 * its sink pad.  Default is that max-duration-time is not specified.
 *
 * #GstLooper:start-time is the offset from the beginning of the input to 
 * start the output, in nanoseconds.  Sound before start-time is not sent
 * downstream unless loop-to is before start-time.  Default is 0.
 *
 * #GstLooper:autostart.  Normally, this element sends silence until it 
 * receives a Start message.  By setting the Autostart parameter to TRUE you 
 * can make it start as soon as it has gotten all the sound data it needs.
 * Default is FALSE.
 *
 * #GstLooper:file-location.  This element will attempt to use pull mode to get 
 * sound data as quickly as possible from its upstream source, but fall back to 
 * push mode if necessary.  An even faster alternative to getting the data in 
 * pull mode is to specify the file-location parameter.  Gstlooper will read 
 * the data segements from that file rather than wait for the data to come from 
 * upstream.  The metadata will still come from upstream.  The specified file 
 * must be a WAV file.  Default is that file-location is not specified, so
 * no file is read.
 *
 * Receipt of a Release message causes looping to terminate, which means 
 * reaching the end of the loop no longer causes sound to be sent from the 
 * beginning of the loop.  The amount of sound sent after a Release message can 
 * be as little as 0, if the looper element was about to loop, and there is no 
 * sound after the loop-from time.  Therefore, if you need sound after the 
 * Release message, leave enough sound after loop-from to handle the worst case.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch-1.0 -v -m audiotestsrc ! audio/x-raw,rate=96000,format=S32LE ! looper start-time=1000000000 max-duration=5000000000 loop-from=1000000000 loop-limit=2 autostart=TRUE ! fakesink silent=TRUE
 * ]|
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <math.h>
#include <errno.h>
#include <stdio.h>
#include <gst/gst.h>
#include <gst/audio/audio.h>

#include "gstlooper.h"

#define STATIC_CAPS \
  GST_STATIC_CAPS (GST_AUDIO_CAPS_MAKE \
		   (" { S8, U8, " GST_AUDIO_NE (S16) "," GST_AUDIO_NE (S32) \
		    "," GST_AUDIO_NE (F32) "," GST_AUDIO_NE (F64)" } ") \
		   ", layout = (string) interleaved")
#define SINK_TEMPLATE \
  GST_STATIC_PAD_TEMPLATE ("sink", GST_PAD_SINK, GST_PAD_ALWAYS, STATIC_CAPS)
#define SRC_TEMPLATE							\
  GST_STATIC_PAD_TEMPLATE ("src", GST_PAD_SRC, GST_PAD_ALWAYS, STATIC_CAPS)

static GstStaticPadTemplate sinktemplate = SINK_TEMPLATE;
static GstStaticPadTemplate srctemplate = SRC_TEMPLATE;

GST_DEBUG_CATEGORY_STATIC (looper);
#define GST_CAT_DEFAULT (looper)

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
  PROP_LOOP_LIMIT,
  PROP_MAX_DURATION,
  PROP_START_TIME,
  PROP_AUTOSTART,
  PROP_FILE_LOCATION,
  PROP_ELAPSED_TIME,
  PROP_REMAINING_TIME
};

#define DEBUG_INIT \
  GST_DEBUG_CATEGORY_INIT (looper, "looper", 0, \
			   "Repeat a section of the stream");
#define gst_looper_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE (GstLooper, gst_looper, GST_TYPE_ELEMENT, DEBUG_INIT);

/* Forward declarations  */

/* deallocate */
static void gst_looper_finalize (GObject * object);

/* set the value of a property */
static void gst_looper_set_property (GObject * object, guint prop_id,
                                     const GValue * value,
                                     GParamSpec * pspec);
/* fetch the value of a property */
static void gst_looper_get_property (GObject * object, guint prop_id,
                                     GValue * value, GParamSpec * pspec);
/* process incoming data from the sink pad */
static void gst_looper_pull_data_from_upstream (GstPad * pad);
static GstFlowReturn gst_looper_chain (GstPad * pad, GstObject * parent,
                                       GstBuffer * buffer);
/* send outgoing data to the source pad */
static void gst_looper_push_data_downstream (GstPad * pad);
/* process events and querys on the sink and source pads */
static gboolean gst_looper_handle_sink_event (GstPad * pad,
                                              GstObject * parent,
                                              GstEvent * event);
static gboolean gst_looper_handle_sink_query (GstPad * pad,
                                              GstObject * parent,
                                              GstQuery * query);
static gboolean gst_looper_handle_src_event (GstPad * pad, GstObject * parent,
                                             GstEvent * event);
static gboolean gst_looper_handle_src_query (GstPad * pad, GstObject * parent,
                                             GstQuery * query);
/* process querys directed to the element itself */
static gboolean gst_looper_handle_query (GstElement * element,
                                         GstQuery * query);
/* send data downstream in pull mode */
static GstFlowReturn gst_looper_get_range (GstPad * pad, GstObject * parent,
                                           guint64 offset, guint length,
                                           GstBuffer ** buffer);
/* activate and deactivate the source and sink pads */
static gboolean gst_looper_activate_sink_pad (GstPad * pad,
                                              GstObject * parent);
static gboolean gst_looper_src_activate_mode (GstPad * pad,
                                              GstObject * parent,
                                              GstPadMode mode,
                                              gboolean active);
static gboolean gst_looper_sink_activate_mode (GstPad * pad,
                                               GstObject * parent,
                                               GstPadMode mode,
                                               gboolean active);
/* process a state change */
static GstStateChangeReturn gst_looper_change_state (GstElement * element,
                                                     GstStateChange
                                                     transition);

/* local subroutines */

/* convert a time in nanoseconds into a position in the buffer */
static guint64 round_up_to_position (GstLooper * self,
                                     guint64 specified_time);
static guint64 round_down_to_position (GstLooper * self,
                                       guint64 specified_time);

/* Read the data chunks from a WAV file into the local buffer.  */
static gboolean read_wav_file_data (GstLooper * self, guint64 max_position);

/* GObject vmethod implementations */

/* initialize the looper's class */
static void
gst_looper_class_init (GstLooperClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GParamSpec *param_spec;
  gchar *string_default;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  gobject_class->set_property = gst_looper_set_property;
  gobject_class->get_property = gst_looper_get_property;

  /* Properties */
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

  param_spec =
    g_param_spec_uint64 ("max-duration", "Max_duration",
                         "Maximum time to accept from upstream", 0,
                         G_MAXUINT64, 0, G_PARAM_READWRITE);
  g_object_class_install_property (gobject_class, PROP_MAX_DURATION,
                                   param_spec);

  param_spec =
    g_param_spec_uint64 ("start-time", "Start_time",
                         "Offset from the start to begin outputting", 0,
                         G_MAXUINT64, 0, G_PARAM_READWRITE);
  g_object_class_install_property (gobject_class, PROP_START_TIME,
                                   param_spec);

  param_spec =
    g_param_spec_boolean ("autostart", "Autostart", "automatic start", FALSE,
                          G_PARAM_READWRITE);
  g_object_class_install_property (gobject_class, PROP_AUTOSTART, param_spec);

  string_default = g_strdup ("");
  param_spec =
    g_param_spec_string ("file-location", "File_location",
                         "The location of the WAV file "
                         "for fast loading of data", string_default,
                         G_PARAM_READWRITE);
  g_object_class_install_property (gobject_class, PROP_FILE_LOCATION,
                                   param_spec);

  param_spec =
    g_param_spec_string ("elapsed-time", "elapsed_time",
                         "Time in seconds since the sound was started",
                         string_default, G_PARAM_READABLE);
  g_object_class_install_property (gobject_class, PROP_ELAPSED_TIME,
                                   param_spec);

  param_spec =
    g_param_spec_string ("remaining-time", "remaining_time",
                         "Time in seconds until the sound stops",
                         string_default, G_PARAM_READABLE);
  g_object_class_install_property (gobject_class, PROP_REMAINING_TIME,
                                   param_spec);

  g_free (string_default);
  string_default = NULL;

  /* Set several parent class virtual functions.  */
  gobject_class->finalize = gst_looper_finalize;
  gst_element_class_add_pad_template (gstelement_class,
                                      gst_static_pad_template_get
                                      (&srctemplate));
  gst_element_class_add_pad_template (gstelement_class,
                                      gst_static_pad_template_get
                                      (&sinktemplate));
  gst_element_class_set_static_metadata (gstelement_class, "Looper",
                                         "Generic",
                                         "Repeat a section of the input stream",
                                         "John Sauter <John_Sauter@"
                                         "systemeyescomputerstore.com>");
  gstelement_class->change_state =
    GST_DEBUG_FUNCPTR (gst_looper_change_state);
  gstelement_class->query = GST_DEBUG_FUNCPTR (gst_looper_handle_query);
}

/* initialize the new element
 * initialize instance structure
 */
static void
gst_looper_init (GstLooper * self)
{

  self->silent = FALSE;
  self->loop_to = 0;
  self->loop_from = 0;
  self->loop_limit = 0;
  self->loop_counter = 0;
  self->timestamp_offset = 0;
  self->max_duration = 0;
  self->start_time = 0;
  self->autostart = FALSE;
  self->started = FALSE;
  self->completion_sent = FALSE;
  self->paused = FALSE;
  self->continued = FALSE;
  self->released = FALSE;
  self->data_buffered = FALSE;
  self->local_buffer = gst_buffer_new ();
  self->local_buffer_fill_level = 0;
  self->local_buffer_drain_level = 0;
  self->pull_level = 0;
  self->local_buffer_size = 0;
  self->bytes_per_ns = 0.0;
  self->local_clock = 0;
  self->elapsed_time = 0;
  self->width = 0;
  self->channel_count = 0;
  self->format = NULL;
  self->data_rate = 0;
  self->send_EOS = FALSE;
  self->state_change_pending = FALSE;
  self->src_pad_task_running = FALSE;
  self->sink_pad_task_running = FALSE;
  self->file_location = NULL;
  self->file_location_specified = FALSE;
  self->seen_incoming_data = FALSE;
  g_rec_mutex_init (&self->interlock);
  self->silence_byte = 0;

  /* create the pads */
  self->sinkpad = gst_pad_new_from_static_template (&sinktemplate, "sink");
  gst_pad_set_chain_function (self->sinkpad,
                              GST_DEBUG_FUNCPTR (gst_looper_chain));
  gst_pad_set_activate_function (self->sinkpad, gst_looper_activate_sink_pad);
  gst_pad_set_activatemode_function (self->sinkpad,
                                     GST_DEBUG_FUNCPTR
                                     (gst_looper_sink_activate_mode));
  gst_pad_set_event_function (self->sinkpad,
                              GST_DEBUG_FUNCPTR
                              (gst_looper_handle_sink_event));
  gst_pad_set_query_function (self->sinkpad,
                              GST_DEBUG_FUNCPTR
                              (gst_looper_handle_sink_query));
  /* Set the pad to forward all caps-related events and queries to its
   * peer pad on the upstream element.  This implies that incoming data
   * and outgoing data will have the same format.  */
  GST_PAD_SET_PROXY_CAPS (self->sinkpad);
  gst_element_add_pad (GST_ELEMENT (self), self->sinkpad);

  self->srcpad = gst_pad_new_from_static_template (&srctemplate, "src");
  gst_pad_set_activatemode_function (self->srcpad,
                                     GST_DEBUG_FUNCPTR
                                     (gst_looper_src_activate_mode));
  gst_pad_set_getrange_function (self->srcpad,
                                 GST_DEBUG_FUNCPTR (gst_looper_get_range));
  gst_pad_set_event_function (self->srcpad,
                              GST_DEBUG_FUNCPTR
                              (gst_looper_handle_src_event));
  gst_pad_set_query_function (self->srcpad,
                              GST_DEBUG_FUNCPTR
                              (gst_looper_handle_src_query));
  /* Set the pad to forward all caps-related events and queries to its
   * peer pad on the downstream element.  This implies that incoming data
   * and outgoing data will have the same format.  */
  GST_PAD_SET_PROXY_CAPS (self->srcpad);
  gst_element_add_pad (GST_ELEMENT (self), self->srcpad);

  return;
}

/* Deallocate everything */
static void
gst_looper_finalize (GObject * object)
{
  GstLooper *self = GST_LOOPER (object);

  if (self->local_buffer != NULL)
    {
      gst_buffer_unref (self->local_buffer);
      self->local_buffer = NULL;
    }
  if (self->format != NULL)
    {
      g_free (self->format);
      self->format = NULL;
    }
  if (self->file_location != NULL)
    {
      g_free (self->file_location);
      self->file_location = NULL;
      self->file_location_specified = FALSE;
    }
  g_rec_mutex_clear (&self->interlock);
  G_OBJECT_CLASS (parent_class)->finalize (object);
  return;
}

/* Process a state change.  The state either climbs up from NULL to READY
 * to PAUSED to RUNNING, or down from RUNNING to PAUSED to READY to NULL.   */
static GstStateChangeReturn
gst_looper_change_state (GstElement * element, GstStateChange transition)
{
  GstLooper *self;
  GstStateChangeReturn result = GST_STATE_CHANGE_SUCCESS;

  self = GST_LOOPER (element);

  switch (transition)
    {
    case GST_STATE_CHANGE_NULL_TO_READY:
      GST_DEBUG_OBJECT (self, "state changed from null to ready");
      break;

    case GST_STATE_CHANGE_READY_TO_PAUSED:
      g_rec_mutex_lock (&self->interlock);
      self->started = FALSE;
      self->completion_sent = FALSE;
      self->released = FALSE;
      self->paused = FALSE;
      self->continued = FALSE;
      self->data_buffered = FALSE;
      GST_DEBUG_OBJECT (self, "state changed from ready to paused");
      g_rec_mutex_unlock (&self->interlock);
      break;

    case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
      g_rec_mutex_lock (&self->interlock);
      if ((self->data_buffered) && (!self->src_pad_task_running))
        {
          /* Start the task which pushes data downstream.  */
          result =
            gst_pad_start_task (self->srcpad,
                                (GstTaskFunction)
                                gst_looper_push_data_downstream, self->srcpad,
                                NULL);
          if (!result)
            {
              GST_DEBUG_OBJECT (self,
                                "failed to start push task after state change");
            }
          self->src_pad_task_running = TRUE;
        }
      GST_DEBUG_OBJECT (self, "state changed from paused to playing");
      g_rec_mutex_unlock (&self->interlock);
      break;

    default:
      break;
    }

  /* Let our parent do its state change after us if we are going up,
   * or before us if we are going down.  */
  result =
    GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  if (result == GST_STATE_CHANGE_FAILURE)
    {
      GST_DEBUG_OBJECT (self, "failure of parent during state change");
      return result;
    }

  switch (transition)
    {
    case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
      g_rec_mutex_lock (&self->interlock);

      /* The pipeline is pausing.  If the task that sends data
       * downstream is still running, tell it to send EOS and
       * complete the state transition.  If it is not running,
       * complete the state transition here.  */

      if (self->src_pad_task_running)
        {
          self->send_EOS = TRUE;
          result = GST_STATE_CHANGE_ASYNC;
          self->state_change_pending = TRUE;
          GST_DEBUG_OBJECT (self, "state changing from playing to paused");
        }
      else
        {
          GST_DEBUG_OBJECT (self, "state changed from playing to paused");
        }

      g_rec_mutex_unlock (&self->interlock);
      break;

    case GST_STATE_CHANGE_PAUSED_TO_READY:
      g_rec_mutex_lock (&self->interlock);
      /* If the tasks that are pushing data downstream or pulling data from
       * upstream are still running, kill them.  */
      if (self->src_pad_task_running)
        {
          gst_pad_stop_task (self->srcpad);
          self->src_pad_task_running = FALSE;
        }
      if (self->sink_pad_task_running)
        {
          gst_pad_stop_task (self->sinkpad);
          self->sink_pad_task_running = FALSE;
        }
      self->data_buffered = FALSE;
      self->started = FALSE;
      self->completion_sent = FALSE;
      self->paused = FALSE;
      self->continued = FALSE;
      self->released = FALSE;
      GST_DEBUG_OBJECT (self, "state changed from paused to ready");
      g_rec_mutex_unlock (&self->interlock);
      break;

    case GST_STATE_CHANGE_READY_TO_NULL:
      GST_DEBUG_OBJECT (self, "state changed from ready to null");
      break;

    default:
      break;
    }

  return result;
}

/* Activate function for the sink pad.  See if pull mode is supported, use push 
 * mode if it isn't.  */
static gboolean
gst_looper_activate_sink_pad (GstPad * pad, GstObject * parent)
{
  GstLooper *self = GST_LOOPER (parent);
  GstQuery *query;
  gboolean pull_mode_supported, result;

  GST_DEBUG_OBJECT (self, "activating sink pad");

  /* First check what upstream scheduling is supported */
  query = gst_query_new_scheduling ();
  result = gst_pad_peer_query (pad, query);
  if (result)
    {
      /* See if pull mode is supported.  */
      GST_DEBUG_OBJECT (self, "checking upstream peer for being seekable");
      pull_mode_supported =
        gst_query_has_scheduling_mode_with_flags (query, GST_PAD_MODE_PULL,
                                                  GST_SCHEDULING_FLAG_SEEKABLE);
    }
  else
    {
      pull_mode_supported = FALSE;
    }
  gst_query_unref (query);

  if (pull_mode_supported)
    {
      /* We can activate in pull mode.  Gstreamer will also activate the
       * upstream peer in pull mode.  */
      GST_DEBUG_OBJECT (self, "will activate sink pad in pull mode");
      return gst_pad_activate_mode (pad, GST_PAD_MODE_PULL, TRUE);
    }

  GST_INFO_OBJECT (self, "falling back to push mode");
  /* The upstream peer does not support pull mode, so we activate in push
   * mode as a fallback.  */
  return gst_pad_activate_mode (pad, GST_PAD_MODE_PUSH, TRUE);
}

/* Activate or deactivate the source pad in either push or pull mode.  Note
 * that pull mode is not fully implemented.  */
static gboolean
gst_looper_src_activate_mode (GstPad * pad, GstObject * parent,
                              GstPadMode mode, gboolean active)
{
  GstLooper *self = GST_LOOPER (parent);
  gboolean result;

  switch (mode)
    {
    case GST_PAD_MODE_PULL:
      g_rec_mutex_lock (&self->interlock);
      /* The source pad is operating in pull mode.  Downstream will call our
       * getrange function to get data.  */
      if (active)
        {
          GST_DEBUG_OBJECT (self, "activating source pad in pull mode");
          result = TRUE;
          self->src_pad_mode = mode;
          self->src_pad_active = TRUE;
        }
      else
        {
          GST_DEBUG_OBJECT (self, "deactivating source pad in pull mode");
          self->src_pad_mode = mode;
          self->src_pad_active = FALSE;
          result = TRUE;
        }
      g_rec_mutex_unlock (&self->interlock);
      break;

    case GST_PAD_MODE_PUSH:
      g_rec_mutex_lock (&self->interlock);
      /* The source pad is operating in push mode.  To activate we will
       * start a task which pushes out buffers when our local buffer is full.  
       */
      if (active)
        {
          GST_DEBUG_OBJECT (self, "activating source pad in push mode");
          self->src_pad_mode = mode;
          if ((self->data_buffered) && (!self->src_pad_task_running))
            {
              /* Start the task which pushes data downstream.  */
              result =
                gst_pad_start_task (self->srcpad,
                                    (GstTaskFunction)
                                    gst_looper_push_data_downstream,
                                    self->srcpad, NULL);
              if (!result)
                {
                  GST_DEBUG_OBJECT (self,
                                    "failed to start push task "
                                    "after pad activate");
                }
              self->src_pad_task_running = TRUE;
            }
          self->src_pad_active = TRUE;
          result = TRUE;
        }
      else
        {
          GST_DEBUG_OBJECT (self, "deactivating source pad in push mode");
          self->src_pad_mode = mode;
          self->src_pad_active = FALSE;
          /* If the task that is sending data downstream is still running, 
           * have it send EOS and terminate.  */
          self->send_EOS = TRUE;
          result = TRUE;
        }
      g_rec_mutex_unlock (&self->interlock);
      break;

    default:
      GST_DEBUG_OBJECT (pad, "unknown source pad activation mode: %d.", mode);
      result = FALSE;
      break;
    }

  return result;
}

/* Called repeatedly with @pad as the source pad.  This function pushes out
 * data to the downstream peer element.  */
static void
gst_looper_push_data_downstream (GstPad * pad)
{
  GstLooper *self = GST_LOOPER (GST_PAD_PARENT (pad));
  GstBuffer *buffer;
  GstEvent *event;
  GstStructure *structure;
  GstMemory *memory_out;
  GstMapInfo memory_in_info, memory_out_info;
  gsize data_size;
  gboolean result, within_loop;
  GstFlowReturn flow_result;
  gboolean send_silence;
  gboolean buffer_complete;
  gboolean exiting = FALSE;
  guint64 loop_from_position, loop_to_position;

  /* We have a recursive mutex which prevents this task from running
   * while some other part of this plugin is running on a different task.  
   * We take the interlock here, therefore waiting for the other task to 
   * release it.  Before we exit we must release this mutex or the plugin 
   * will grind to a halt.  */
  g_rec_mutex_lock (&self->interlock);

  /* If we should not be running, just exit.  */
  if (!self->src_pad_task_running)
    {
      GST_DEBUG_OBJECT (self, "data pusher should not be running");
      g_rec_mutex_unlock (&self->interlock);
      return;
    }

  /* If requested, or if we are autostarted and have reached the end of
   * the buffer, send an end-of-stream message and stop.  */
  if ((self->send_EOS)
      || (self->autostart
          && (self->local_buffer_drain_level >= self->local_buffer_size)))
    {
      GST_INFO_OBJECT (self, "pushing an EOS event");
      event = gst_event_new_eos ();
      result = gst_pad_push_event (self->srcpad, event);
      if (!result)
        {
          GST_DEBUG_OBJECT (self, "failed to push an EOS event");
        }
      self->send_EOS = FALSE;

      /* Having pushed an EOS event, we are done.  */
      GST_DEBUG_OBJECT (self, "pausing source pad task");
      result = gst_pad_pause_task (self->srcpad);
      if (!result)
        {
          GST_DEBUG_OBJECT (self, "failed to pause source pad task");
        }

      self->src_pad_task_running = FALSE;
      exiting = TRUE;
    }

  /* If we are making the transition from the playing to the paused
   * state, complete the transition here.
   */
  if (self->state_change_pending)
    {
      GST_DEBUG_OBJECT (self, "completing state change");
      gst_element_continue_state (GST_ELEMENT (self),
                                  GST_STATE_CHANGE_SUCCESS);
      GST_DEBUG_OBJECT (self, "state change completed");
      self->state_change_pending = FALSE;
      exiting = TRUE;
    }

  /* If we either sent an EOS, or completed a state change, or both,
   * exit now.  */
  if (exiting)
    {
      g_rec_mutex_unlock (&self->interlock);
      return;
    }

  /* If we are flushing, do not push any data.  */
  if (self->src_pad_flushing)
    {
      GST_DEBUG_OBJECT (self, "data pusher should not run while flushing");
      g_rec_mutex_unlock (&self->interlock);
      return;
    }

  /* If we were paused but have since received a continue message,
   * stop pausing.  */
  if (self->paused && self->continued)
    {
      self->paused = FALSE;
      self->continued = FALSE;
    }

  /* If we have not received a start event, or if we have completely
   * drained the buffer, or we are paused, remember to send silence 
   * downstream.  */
  send_silence = FALSE;
  buffer_complete = FALSE;
  if (!self->started)
    {
      send_silence = TRUE;
    }
  else
    {
      if (self->local_buffer_drain_level >= self->local_buffer_size)
        {
          send_silence = TRUE;
          buffer_complete = TRUE;
        }
    }
  if (self->paused && !self->continued)
    send_silence = TRUE;

  /* If we are just reaching the end of the buffer, and we are not
   * autostarted, send a message downstream to the envelope plugin, 
   * so it knows that the sound is complete.  We don't send EOS because 
   * we don't want to drain the pipeline--we may get another Start message 
   * asking us to play this sound again.  Note that, if we are
   * autostarted, we sent an EOS message above and don't get here.  */
  if (self->started && buffer_complete && !self->completion_sent)
    {
      GST_DEBUG_OBJECT (self, "pushing a completion event");
      structure = gst_structure_new_empty ((gchar *) "complete");
      event = gst_event_new_custom (GST_EVENT_CUSTOM_DOWNSTREAM, structure);
      result = gst_pad_push_event (self->srcpad, event);
      if (!result)
        {
          GST_DEBUG_OBJECT (self, "failed to push a completion event");
        }
      else
        {
          GST_DEBUG_OBJECT (self, "successfully pushed a completion event");
        }
      self->completion_sent = TRUE;
    }

  if (send_silence)
    {
      GST_DEBUG_OBJECT (self, "sending silence downstream");
      /* Compute the number of bytes required to hold 40 milliseconds
       * of silence.  */
      data_size =
        self->width * self->data_rate * self->channel_count / (8000 / 40);
      /* Allocate that much memory, and place it in our output buffer.  */
      memory_out = gst_allocator_alloc (NULL, data_size, NULL);
      buffer = gst_buffer_new ();
      gst_buffer_append_memory (buffer, memory_out);
      /* Fill the buffer with the silence byte.  */
      gst_buffer_map (buffer, &memory_out_info, GST_MAP_WRITE);
      gst_buffer_memset (buffer, 0, self->silence_byte, memory_out_info.size);
      /* Set the time stamps in the buffer.  */
      GST_BUFFER_PTS (buffer) = self->local_clock;
      GST_BUFFER_DTS (buffer) = self->local_clock;
      GST_BUFFER_DURATION (buffer) =
        memory_out_info.size / self->bytes_per_ns;
      /* Advance our clock.  */
      self->local_clock =
        self->local_clock + (memory_out_info.size / self->bytes_per_ns);
      GST_BUFFER_OFFSET (buffer) = self->local_buffer_drain_level;
      GST_BUFFER_OFFSET_END (buffer) =
        self->local_buffer_drain_level + memory_out_info.size;
      gst_buffer_unmap (buffer, &memory_out_info);
      /* Send the buffer downstream.  */
      GST_DEBUG_OBJECT (self,
                        "pushing %" G_GUINT64_FORMAT " bytes of silence.",
                        data_size);
      /* We must unlock before we push, since pushing can cause a query to 
       * come back upstream on a different task before it completes.  */
      g_rec_mutex_unlock (&self->interlock);

      flow_result = gst_pad_push (self->srcpad, buffer);
      if (flow_result != GST_FLOW_OK)
        {
          GST_DEBUG_OBJECT (self, "pad push of silence returned %s",
                            gst_flow_get_name (flow_result));
        }

      GST_DEBUG_OBJECT (self, "push of silence completed");
      return;
    }

  /* There is more data to send.  Allocate a new buffer to send downstream, 
   * and copy data from our local buffer to it.  */
  buffer = gst_buffer_new ();
  /* We send 40 milliseconds of buffer data at a time, but not more than
   * is left in our local buffer, and not more than we need to reach the
   * end of the loop, if we are looping.  */
  data_size =
    self->width * self->data_rate * self->channel_count / (8000 / 40);
  if (data_size > self->local_buffer_size - self->local_buffer_drain_level)
    {
      data_size = self->local_buffer_size - self->local_buffer_drain_level;
    }

  /* We are within the loop if this isn't our last time around.  */
  within_loop = FALSE;
  loop_from_position = round_up_to_position (self, self->loop_from);
  if ((!self->released) && (self->loop_from > 0)
      && (self->local_buffer_drain_level <= loop_from_position))
    {
      if ((self->loop_limit == 0) || (self->loop_counter < self->loop_limit))
        {
          within_loop = TRUE;
        }
    }
  /* If we are within the loop but at the very end, go back to the beginning.
   */
  if (within_loop && (self->local_buffer_drain_level == loop_from_position))
    {
      loop_to_position = round_down_to_position (self, self->loop_to);
      self->local_buffer_drain_level = loop_to_position;
      self->loop_counter = self->loop_counter + 1;
      GST_DEBUG_OBJECT (self,
                        "loop counter %" G_GUINT64_FORMAT ", looping from %"
                        GST_TIME_FORMAT " to %" GST_TIME_FORMAT ".",
                        self->loop_counter,
                        GST_TIME_ARGS (loop_from_position /
                                       self->bytes_per_ns),
                        GST_TIME_ARGS (loop_to_position /
                                       self->bytes_per_ns));
    }
  /* If the loop is very short, we will output buffers of its length.  */
  if (within_loop
      && (data_size > loop_from_position - self->local_buffer_drain_level))
    {
      data_size = loop_from_position - self->local_buffer_drain_level;
    }
  /* now that we know how much memory we need, allocate it */
  memory_out = gst_allocator_alloc (NULL, data_size, NULL);
  /* place the memory in the output buffer and mark it for writing */
  gst_buffer_append_memory (buffer, memory_out);
  gst_buffer_map (buffer, &memory_out_info, GST_MAP_WRITE);
  /* mark our local buffer for reading */
  gst_buffer_map (self->local_buffer, &memory_in_info, GST_MAP_READ);
  /* copy data from our local buffer to the output buffer */
  gst_buffer_fill (buffer, 0,
                   memory_in_info.data + self->local_buffer_drain_level,
                   memory_out_info.size);

  /* Set the time stamps in the output buffer.  */
  GST_BUFFER_PTS (buffer) = self->local_clock;
  GST_BUFFER_DTS (buffer) = self->local_clock;
  GST_BUFFER_DURATION (buffer) = memory_out_info.size / self->bytes_per_ns;
  /* Advance our clock.  */
  self->local_clock =
    self->local_clock + (memory_out_info.size / self->bytes_per_ns);
  /* Keep track of the amount of time we have been sending sound.  */
  self->elapsed_time =
    self->elapsed_time + (memory_out_info.size / self->bytes_per_ns);
  GST_DEBUG_OBJECT (self, "elapsed time is %" G_GUINT64_FORMAT ".",
                    self->elapsed_time);
  /* Note the byte offsets in the source.  */
  GST_BUFFER_OFFSET (buffer) = self->local_buffer_drain_level;
  GST_BUFFER_OFFSET_END (buffer) =
    self->local_buffer_drain_level + memory_out_info.size;

  GST_DEBUG_OBJECT (self,
                    "sending %" G_GUINT64_FORMAT " bytes of data downstream"
                    " from buffer position %" G_GUINT64_FORMAT ".",
                    memory_out_info.size, self->local_buffer_drain_level);

  /* Update the current position in our local buffer.  */
  self->local_buffer_drain_level =
    self->local_buffer_drain_level + memory_out_info.size;

  /* we are finished with the new buffer and our local buffer */
  gst_buffer_unmap (buffer, &memory_out_info);
  gst_buffer_unmap (self->local_buffer, &memory_in_info);

  /* We must unlock before we push, since pushing can cause a query to come
   * back upstream on another task before it completes.  */
  g_rec_mutex_unlock (&self->interlock);

  flow_result = gst_pad_push (self->srcpad, buffer);
  if (flow_result != GST_FLOW_OK)
    {
      GST_DEBUG_OBJECT ("pad push of data returned with %s.",
                        gst_flow_get_name (flow_result));
    }
  GST_DEBUG_OBJECT (self, "completed push of data");

  return;

}

/* Activate or deactivate the sink pad.  */
static gboolean
gst_looper_sink_activate_mode (GstPad * pad, GstObject * parent,
                               GstPadMode mode, gboolean active)
{
  gboolean result;
  GstLooper *self = GST_LOOPER (parent);

  switch (mode)
    {
    case GST_PAD_MODE_PUSH:
      g_rec_mutex_lock (&self->interlock);
      if (active)
        {
          GST_INFO_OBJECT (self, "activating sink pad in push mode");
          self->sink_pad_mode = mode;
          self->sink_pad_active = TRUE;
        }
      else
        {
          GST_INFO_OBJECT (self, "deactivating sink pad in push mode");
          self->sink_pad_mode = mode;
          self->sink_pad_active = FALSE;
        }
      g_rec_mutex_unlock (&self->interlock);
      result = TRUE;
      break;

    case GST_PAD_MODE_PULL:
      g_rec_mutex_lock (&self->interlock);
      if (active)
        {
          GST_INFO_OBJECT (self, "activating sink pad in pull mode");
          self->sink_pad_mode = mode;

          /* Start the task that will pull data from upstream into the local
           * buffer.  */
          if (!self->sink_pad_task_running)
            {
              result =
                gst_pad_start_task (self->sinkpad,
                                    (GstTaskFunction)
                                    gst_looper_pull_data_from_upstream,
                                    self->sinkpad, NULL);
              if (!result)
                {
                  GST_DEBUG_OBJECT (self,
                                    "failed to start task "
                                    "after sink pad activate");
                }
              self->sink_pad_task_running = TRUE;
            }
          result = TRUE;
          self->sink_pad_active = TRUE;
        }
      else
        {
          GST_INFO_OBJECT (self, "deactivating sink pad in pull mode");
          self->sink_pad_mode = mode;

          /* If it is still running, stop the task that is pulling data from
           * upstream into the local buffer.  */
          if (self->sink_pad_task_running)
            {
              gst_pad_stop_task (pad);
              self->sink_pad_task_running = FALSE;
            }
          self->sink_pad_active = FALSE;
        }
      g_rec_mutex_unlock (&self->interlock);
      result = TRUE;
      break;

    default:
      GST_DEBUG_OBJECT (pad, "unknown sink pad activation mode: %d.", mode);
      result = FALSE;
      break;
    }

  return result;
}

/* Pull sound data from upstream.  Called repeatedly with @pad as the sink pad.
 */
static void
gst_looper_pull_data_from_upstream (GstPad * pad)
{
  GstLooper *self = GST_LOOPER (GST_PAD_PARENT (pad));
  GstMemory *memory_allocated;
  GstMapInfo memory_in_info, buffer_memory_info;
  guint64 byte_offset;
  char *byte_data_in_pointer, *byte_data_out_pointer;
  gboolean result, pull_result;
  guint64 max_position, start_position;
  gboolean max_duration_reached;
  GstBuffer *pull_buffer = NULL;

  /* This subroutine runs as its own task, so prevent destructive interference
   * with the local data by waiting until no other task is running any code
   * in this element.  We must be careful to release this interlock when
   * we exit.  */
  g_rec_mutex_lock (&self->interlock);

  /* If we should not be running, just exit.  */
  if (!self->sink_pad_task_running)
    {
      GST_DEBUG_OBJECT (self, "data puller should not be running");
      g_rec_mutex_unlock (&self->interlock);
      return;
    }

  /* If the local buffer has been filled, and we have already seen some data,
   * we don't need to run any more.  */
  if ((self->data_buffered) && (self->seen_incoming_data))
    {
      GST_DEBUG_OBJECT (self, "pausing sink pad task");
      result = gst_pad_pause_task (self->sinkpad);
      if (!result)
        {
          GST_DEBUG_OBJECT (self, "failed to pause sink pad task");
        }
      self->sink_pad_task_running = FALSE;
      g_rec_mutex_unlock (&self->interlock);
      return;
    }

  /* Ask our upstream peer to give us some data.  */
  pull_result =
    gst_pad_pull_range (pad, self->pull_level, BUFFER_SIZE, &pull_buffer);
  if (pull_result == GST_FLOW_OK)
    {
      GST_DEBUG_OBJECT (self,
                        "received buffer %p of size %" G_GSIZE_FORMAT ".",
                        pull_buffer, gst_buffer_get_size (pull_buffer));

      /* If this is the first time we have seen any data from upstream, but
       * we already have all our data, which can only be true if we read
       * the data directly from the WAV file, start pushing data downstream.  */
      if ((self->data_buffered) && (!self->seen_incoming_data))
        {
          /* Begin pushing data from our local buffer downstream using the
           * source pad.  Unless we are autostarted, that task will send 
           * silence until we get a Start message.  */
          if (!self->src_pad_task_running)
            {
              result =
                gst_pad_start_task (self->srcpad,
                                    (GstTaskFunction)
                                    gst_looper_push_data_downstream,
                                    self->srcpad, NULL);
              self->src_pad_task_running = TRUE;
            }
        }
      self->seen_incoming_data = TRUE;

      /* If our local buffer has already been filled, we have no need for
       * this additional data.  The next time around we will pause this task. 
       */
      if (self->data_buffered)
        {
          gst_buffer_unref (pull_buffer);
          g_rec_mutex_unlock (&self->interlock);
          return;
        }
    }

  /* See if we have already reached max-duration.  If so, we have no need
   * for this data buffer.  */
  max_duration_reached = FALSE;
  max_position = 0;
  if (self->max_duration > 0)
    {
      max_position = round_up_to_position (self, self->max_duration);
      if (self->local_buffer_fill_level > max_position)
        {
          max_duration_reached = TRUE;
        }
    }

  if (max_duration_reached || (pull_result != GST_FLOW_OK))
    {
      /* Either we got an error trying to pull sound data from upstream,
       * or we have reached max-duration.  In either case we have finished 
       * pulling sound data.  */

      /* If we have a buffer from upstream, discard it.  */
      if (pull_result == GST_FLOW_OK)
        {
          gst_buffer_unref (pull_buffer);
        }

      GST_INFO_OBJECT (self,
                       "stopped pulling sound data at offset %"
                       G_GUINT64_FORMAT ".", self->local_buffer_fill_level);
      self->data_buffered = TRUE;
      /* We now know the size of our local buffer.  We may have filled it
       * a little beyond max-duration, but if so we will use only the data
       * up to max-duration.  */
      if (self->max_duration > 0
          && max_position < self->local_buffer_fill_level)
        {
          self->local_buffer_size = max_position;
        }
      else
        {
          self->local_buffer_size = self->local_buffer_fill_level;
        }

      /* Set the position from which to start draining the buffer.  */
      start_position = round_down_to_position (self, self->start_time);
      self->local_buffer_drain_level = start_position;

      /* If the Autostart parameter has been set to TRUE, don't wait
       * for a Start event.  */
      if (self->autostart)
        {
          self->started = TRUE;
          self->local_clock = 0;
          self->elapsed_time = 0;
        }
      /* Begin pushing data from our local buffer downstream using the
       * source pad.  Unless we are autostarted, that task will send silence 
       * until we get a Start message.  */
      if (!self->src_pad_task_running)
        {
          result =
            gst_pad_start_task (self->srcpad,
                                (GstTaskFunction)
                                gst_looper_push_data_downstream, self->srcpad,
                                NULL);
          self->src_pad_task_running = TRUE;
        }
    }
  /* We are done.  The next time around we will detect that the buffer has
   * been filled and stop this task.  */
  g_rec_mutex_unlock (&self->interlock);
  return;

  /* We have a buffer from upstream and we have not already reached 
   * max duration.  Accept the buffer.  */

  /* Allocate more memory at the end of our local buffer, then copy
   * the data in the received buffer to it.  */
  GST_DEBUG_OBJECT (self, "map pulled buffer for reading");
  result = gst_buffer_map (pull_buffer, &memory_in_info, GST_MAP_READ);
  if (!result)
    {
      GST_DEBUG_OBJECT (self, "unable to map pulled buffer for reading");
    }
  memory_allocated = gst_allocator_alloc (NULL, memory_in_info.size, NULL);
  gst_buffer_append_memory (self->local_buffer, memory_allocated);
  result =
    gst_buffer_map (self->local_buffer, &buffer_memory_info, GST_MAP_WRITE);
  if (!result)
    {
      GST_DEBUG_OBJECT (self, "unable to map local buffer for writing");
    }
  GST_DEBUG_OBJECT (self,
                    "copy data from pulled buffer to local buffer"
                    " at %p, memory at %p, offset %" G_GUINT64_FORMAT
                    ", from %p, size %" G_GSIZE_FORMAT ".",
                    self->local_buffer, buffer_memory_info.data,
                    self->local_buffer_fill_level, memory_in_info.data,
                    memory_in_info.size);
  for (byte_offset = 0; byte_offset < memory_in_info.size;
       byte_offset = byte_offset + 1)
    {
      byte_data_in_pointer = (void *) memory_in_info.data + byte_offset;
      byte_data_out_pointer =
        (void *) buffer_memory_info.data + self->local_buffer_fill_level +
        byte_offset;
      *byte_data_out_pointer = *byte_data_in_pointer;
    }
  gst_buffer_unmap (self->local_buffer, &buffer_memory_info);

  /* Update our offset into the local buffer.  */
  self->local_buffer_fill_level =
    self->local_buffer_fill_level + memory_in_info.size;

  /* We are done with the pulled buffer.  */
  gst_buffer_unmap (pull_buffer, &memory_in_info);
  gst_buffer_unref (pull_buffer);

  g_rec_mutex_unlock (&self->interlock);
  return;
}

/* Accept data from upstream if the source pad is in push mode.  We must do this
 * if upstream won't let us pull.  */
static GstFlowReturn
gst_looper_chain (GstPad * pad, GstObject * parent, GstBuffer * buffer)
{
  GstLooper *self = GST_LOOPER (parent);
  GstMemory *memory_allocated;
  GstMapInfo memory_in_info, buffer_memory_info;
  guint64 byte_offset;
  char *byte_data_in_pointer, *byte_data_out_pointer;
  gboolean result;
  guint64 max_position, start_position;
  gboolean max_duration_reached;

  g_rec_mutex_lock (&self->interlock);
  GST_DEBUG_OBJECT (self,
                    "received buffer %p of size %" G_GSIZE_FORMAT ", time %"
                    GST_TIME_FORMAT ", duration %" GST_TIME_FORMAT ".",
                    buffer, gst_buffer_get_size (buffer),
                    GST_TIME_ARGS (GST_BUFFER_TIMESTAMP (buffer)),
                    GST_TIME_ARGS (GST_BUFFER_DURATION (buffer)));

  /* If we have already filled our local buffer, either because we have
   * received max-duration data or we loaded the data directly from the file, 
   * and we have already seen some data, discard any more.  */
  if ((self->data_buffered) && (self->seen_incoming_data))
    {
      /* Discard the buffer from upstream.  */
      gst_buffer_unref (buffer);

      GST_DEBUG_OBJECT (self, "buffer discarded.");

      g_rec_mutex_unlock (&self->interlock);
      return GST_FLOW_OK;
    }

  /* If we have already filled our local buffer, but we are seeing data from
   * upstream for the first time, which can only happen if we filled the
   * buffer by reading sound data directly from the WAV file, start sending
   * sound data from our local buffer downstream.  */
  if ((self->data_buffered) && (!self->seen_incoming_data))
    {
      self->seen_incoming_data = TRUE;
      /* Begin pushing data from our local buffer downstream using the 
       * source pad.  Unless we are autostarted, this task will send 
       * silence until we get a Start message.  */
      result =
        gst_pad_start_task (self->srcpad,
                            (GstTaskFunction) gst_looper_push_data_downstream,
                            self->srcpad, NULL);
      self->src_pad_task_running = TRUE;

      /* Discard the buffer from upstream.  */
      gst_buffer_unref (buffer);

      g_rec_mutex_unlock (&self->interlock);
      return GST_FLOW_OK;
    }

  /* Otherwise, if we have reached max-duration, we have now filled the buffer.
   */
  max_duration_reached = FALSE;
  if (self->max_duration > 0)
    {
      max_position = round_up_to_position (self, self->max_duration);
      if (self->local_buffer_fill_level > max_position)
        {
          max_duration_reached = TRUE;
        }
    }

  if (max_duration_reached)
    {
      /* This is the first time we have received a buffer beyond
       * max-duration, so start sending our buffered data
       * downstream.  */
      GST_INFO_OBJECT (self,
                       "reached max-duration at offset %" G_GUINT64_FORMAT
                       ".", self->local_buffer_fill_level);

      self->data_buffered = TRUE;
      /* We now know the size of our local buffer.  We have filled it
       * a little beyond max-duration, but we will use only max-duration.
       */
      self->local_buffer_size = max_position;
      /* Set the position from which to start draining the buffer.  */
      start_position = round_down_to_position (self, self->start_time);
      self->local_buffer_drain_level = start_position;

      /* If the Autostart parameter has been set to TRUE, don't wait
       * for a Start event.  */
      if (self->autostart)
        {
          self->started = TRUE;
          self->local_clock = 0;
          self->elapsed_time = 0;
        }
      /* Begin pushing data from our local buffer downstream using the 
       * source pad.  Unless we are autostarted, this task will send 
       * silence until we get a Start message.  */
      result =
        gst_pad_start_task (self->srcpad,
                            (GstTaskFunction) gst_looper_push_data_downstream,
                            self->srcpad, NULL);
      self->src_pad_task_running = TRUE;

      /* Discard the buffer from upstream.  */
      gst_buffer_unref (buffer);

      g_rec_mutex_unlock (&self->interlock);
      return GST_FLOW_OK;
    }

  /* Our local buffer has not been filled, and we have not reached max-duration.
   * Allocate more memory at the end of our local buffer, then copy the data in
   * the received buffer to it.  */
  GST_DEBUG_OBJECT (self, "map received buffer for reading");
  result = gst_buffer_map (buffer, &memory_in_info, GST_MAP_READ);
  if (!result)
    {
      GST_DEBUG_OBJECT (self, "unable to map received buffer for reading");
    }
  memory_allocated = gst_allocator_alloc (NULL, memory_in_info.size, NULL);
  gst_buffer_append_memory (self->local_buffer, memory_allocated);
  result =
    gst_buffer_map (self->local_buffer, &buffer_memory_info, GST_MAP_WRITE);
  if (!result)
    {
      GST_DEBUG_OBJECT (self, "unable to map local buffer for writing");
    }
  GST_DEBUG_OBJECT (self,
                    "copy data from received buffer to local buffer"
                    " at %p, memory at %p, offset %" G_GUINT64_FORMAT
                    ", from %p, size %" G_GSIZE_FORMAT ".",
                    self->local_buffer, buffer_memory_info.data,
                    self->local_buffer_fill_level, memory_in_info.data,
                    memory_in_info.size);
  for (byte_offset = 0; byte_offset < memory_in_info.size;
       byte_offset = byte_offset + 1)
    {
      byte_data_in_pointer = (void *) memory_in_info.data + byte_offset;
      byte_data_out_pointer =
        (void *) buffer_memory_info.data + self->local_buffer_fill_level +
        byte_offset;
      *byte_data_out_pointer = *byte_data_in_pointer;
    }
  gst_buffer_unmap (self->local_buffer, &buffer_memory_info);

  /* Update our offset into the local buffer.  */
  self->local_buffer_fill_level =
    self->local_buffer_fill_level + memory_in_info.size;

  /* We are done with the received buffer.  */
  gst_buffer_unmap (buffer, &memory_in_info);
  gst_buffer_unref (buffer);

  g_rec_mutex_unlock (&self->interlock);
  return GST_FLOW_OK;
}

/* Send data downstream in pull mode.  We don't allow pull mode from downstream,
* so this subroutine will never be called.  It is left here in case we decide
* to implement pull mode on the source pad in the future.  */
static GstFlowReturn
gst_looper_get_range (GstPad * pad, GstObject * parent, guint64 offset,
                      guint length, GstBuffer ** buffer)
{
  GstLooper *self = GST_LOOPER (parent);
  GstFlowReturn result = GST_FLOW_OK;
  GstBuffer *buf;
  GstMapInfo memory_info;
  guint64 buf_size;
  gint i;

  g_rec_mutex_lock (&self->interlock);
  GST_DEBUG_OBJECT (self,
                    "Getting range: offset %" G_GUINT64_FORMAT ", length %u",
                    offset, length);

  if (length == -1)
    /* If the buffer length is defaulted, use one millisecond.  */
    {
      buf_size = self->width * self->channel_count * self->data_rate / 1000;
      /* Watch out for the data format not being set up yet.  */
      if (buf_size == 0)
        {
          buf_size = 4096;
        }
    }
  else
    {
      buf_size = length;
    }

  /* If no buffer is passed to get_range, allocate one.  */
  if (*buffer == NULL)
    {
      buf = gst_buffer_new_allocate (NULL, buf_size, NULL);
    }
  else
    {
      buf = *buffer;
    }

  gst_buffer_map (buf, &memory_info, GST_MAP_WRITE);

  /* FIXME: fill the buffer with data from our local buffer.  */
  for (i = 0; i < memory_info.size; i++)
    {
      memory_info.data[i] = 0;
    }
  result = GST_FLOW_OK;

  gst_buffer_unmap (buf, &memory_info);
  gst_buffer_resize (buf, 0, buf_size);
  GST_BUFFER_OFFSET (buf) = 0;
  GST_BUFFER_OFFSET_END (buf) = buf_size;
  *buffer = buf;
  g_rec_mutex_unlock (&self->interlock);
  return result;
}

/* Handle an event arriving at the sink pad.  */
static gboolean
gst_looper_handle_sink_event (GstPad * pad, GstObject * parent,
                              GstEvent * event)
{
  gboolean result = TRUE;
  GstLooper *self = GST_LOOPER (parent);
  GstCaps *in_caps, *out_caps;
  GstStructure *caps_structure;
  gchar *format_code_pointer;
  gchar format_code_0, format_code_1;
  gdouble bits_per_second, bits_per_nanosecond;
  guint64 start_position;
  gint data_rate, channel_count;
  guint64 max_position;
  gboolean wav_file_read;

  GST_DEBUG_OBJECT (self, "received an event on the sink pad");

  switch (GST_EVENT_TYPE (event))
    {
    case GST_EVENT_FLUSH_START:
      g_rec_mutex_lock (&self->interlock);
      GST_LOG_OBJECT (self, "received flush start event on sink pad");
      if (GST_PAD_MODE (self->srcpad) == GST_PAD_MODE_PUSH)
        {
          /* Forward the event downstream.  */
          result = gst_pad_push_event (self->srcpad, event);
          /* Stop the task that is sending data downstream.  */
          gst_pad_stop_task (self->srcpad);
          self->src_pad_task_running = FALSE;
          GST_LOG_OBJECT (self, "loop stopped");
        }
      else
        {
          gst_event_unref (event);
        }
      self->sink_pad_flushing = TRUE;
      g_rec_mutex_unlock (&self->interlock);
      break;

    case GST_EVENT_FLUSH_STOP:
      g_rec_mutex_lock (&self->interlock);
      GST_LOG_OBJECT (self, "received flush stop event on sink pad");
      if (GST_PAD_MODE (self->srcpad) == GST_PAD_MODE_PUSH)
        {
          /* Forward the event downstream.  */
          result = gst_pad_push_event (self->srcpad, event);
          if (!result)
            {
              GST_DEBUG_OBJECT (self, "failed to push flush stop event");
            }
          if ((self->data_buffered) && (!self->src_pad_task_running))
            {
              /* Start the task which pushes data downstream.  */
              result =
                gst_pad_start_task (self->srcpad,
                                    (GstTaskFunction)
                                    gst_looper_push_data_downstream,
                                    self->srcpad, NULL);
              if (!result)
                {
                  GST_DEBUG_OBJECT (self,
                                    "failed to start task after flush stop");
                }
              self->src_pad_task_running = TRUE;
            }
        }
      else
        {
          gst_event_unref (event);
        }
      self->sink_pad_flushing = FALSE;
      g_rec_mutex_unlock (&self->interlock);
      break;

    case GST_EVENT_CAPS:
      /* A caps event on the sink port specifies the format, data rate and
       * number of channels of audio that will come from upstream.  Since we
       * are just passing data through, specify that our source port will
       * use the same format, data rate and number of channels.  */
      g_rec_mutex_lock (&self->interlock);
      gst_event_parse_caps (event, &in_caps);
      GST_DEBUG_OBJECT (self, "input caps are %" GST_PTR_FORMAT ".", in_caps);
      caps_structure = gst_caps_get_structure (in_caps, 0);
      /* Fill in local information about the format, and values based on it.  
       */
      result = gst_structure_get_int (caps_structure, "rate", &data_rate);
      if (!result)
        {
          GST_DEBUG_OBJECT (self, "no rate in caps");
          data_rate = 48000;
        }
      self->data_rate = data_rate;

      result =
        gst_structure_get_int (caps_structure, "channels", &channel_count);
      if (!result)
        {
          GST_DEBUG_OBJECT (self, "no channel count in caps");
          channel_count = 2;
        }
      self->channel_count = channel_count;

      g_free (self->format);
      self->format =
        g_strdup (gst_structure_get_string (caps_structure, "format"));
      if (self->format == NULL)
        {
          GST_DEBUG_OBJECT (self, "no format in caps");
          self->format = g_strdup (GST_AUDIO_NE (F64));
        }

      out_caps =
        gst_caps_new_simple ("audio/x-raw", "format", G_TYPE_STRING,
                             self->format, "rate", G_TYPE_INT,
                             self->data_rate, "channels", G_TYPE_INT,
                             self->channel_count, NULL);
      result = gst_pad_set_caps (self->srcpad, out_caps);
      GST_DEBUG_OBJECT (self, "output caps are %" GST_PTR_FORMAT ".",
                        out_caps);
      gst_caps_unref (out_caps);

      /* Compute the size of a frame from the format string.  
       * The possible formats start with a letter, then the width of
       * sample in bits, for example, F32LE.  The second character of
       * the format can be used to determine the width.  */
      format_code_pointer = self->format;
      format_code_0 = format_code_pointer[0];
      format_code_1 = format_code_pointer[1];
      switch (format_code_1)
        {
        case '8':
          self->width = 8;
          break;
        case '1':
          self->width = 16;
          break;
        case '2':
          self->width = 24;
          break;
        case '3':
          self->width = 32;
          break;
        case '6':
          self->width = 64;
          break;
        default:
          self->width = 32;
          break;
        }
      GST_LOG_OBJECT (self, "second character of format is %c.",
                      format_code_1);
      GST_DEBUG_OBJECT (self, "each sample has %" G_GUINT64_FORMAT " bits.",
                        self->width);

      /* Compute the silence value.  For signed and floating formats, it is 0.
       * for unsigned it is 128 for U8, the only unsigned format we support.  */
      switch (format_code_0)
        {
        case 'S':
        case 'F':
          self->silence_byte = 0;
          break;
        case 'U':
          self->silence_byte = 128;
          break;
        default:
          self->silence_byte = 0;
          break;
        }
      GST_DEBUG_OBJECT (self, "silence value is %hhd.", self->silence_byte);

      /* Compute the data rate in bytes per nanosecond.
       * data_rate times width times channel_count is bits per second.
       * that divided by 1E9 is bits per nanosecond.
       * that divided by 8 is bytes per nanosecond.  */
      bits_per_second = self->data_rate * self->width * self->channel_count;
      bits_per_nanosecond = (gdouble) bits_per_second / (gdouble) 1E9;
      self->bytes_per_ns = bits_per_nanosecond / 8.0;
      GST_DEBUG_OBJECT (self, "data rate is %f bytes per nanosecond.",
                        self->bytes_per_ns);

      /* If a WAV file was specified, this is a good time to read it.  We have
       * the format and data rate, so we can convert max duration 
       * to the maximum size of the local buffer.  */
      if (self->file_location_specified)
        {
          max_position = 0;
          if (self->max_duration > 0)
            {
              max_position = round_up_to_position (self, self->max_duration);
            }

          /* Read the data from the WAV file, up to the most we will need.  */
          wav_file_read = read_wav_file_data (self, max_position);
          if (wav_file_read)
            {
              /* We now have all our data.  */
              self->data_buffered = TRUE;
              GST_DEBUG_OBJECT (self, "read %ld bytes from WAV file.",
                                self->local_buffer_fill_level);
              /* We now know the size of our local buffer.  We may have filled 
                 it beyond max-duration, but if so we will use only the data
                 * up to max-duration.  */
              if (self->max_duration > 0
                  && max_position < self->local_buffer_fill_level)
                {
                  self->local_buffer_size = max_position;
                }
              else
                {
                  self->local_buffer_size = self->local_buffer_fill_level;
                }

              /* Set the position from which to start draining the buffer.  */
              start_position =
                round_down_to_position (self, self->start_time);
              self->local_buffer_drain_level = start_position;

              /* If the Autostart parameter has been set to TRUE, don't wait
               * for a Start event.  */
              if (self->autostart)
                {
                  self->started = TRUE;
                  self->local_clock = 0;
                  self->elapsed_time = 0;
                }
              /* It is too early to start pushing data downstream.  Wait until
               * we get some data from upstream.  */
            }
          else
            {
              GST_DEBUG_OBJECT (self, "read from WAV file failed.");
            }
        }

      g_rec_mutex_unlock (&self->interlock);
      result = gst_pad_push_event (self->srcpad, event);
      break;

    case GST_EVENT_EOS:
      /* We have hit the end of the incoming data stream.  */
      g_rec_mutex_lock (&self->interlock);
      GST_INFO_OBJECT (self,
                       "reached end-of-stream at offset %" G_GUINT64_FORMAT
                       ".", self->local_buffer_fill_level);

      /* If we have already filled the buffer due to reaching max-duration, 
       * we don't need to do anything here.  */
      if (!self->data_buffered)
        {
          self->data_buffered = TRUE;
          /* We now know the size of our local buffer.  */
          self->local_buffer_size = self->local_buffer_fill_level;
          /* Set the initial buffer drain position.  */
          start_position = round_down_to_position (self, self->start_time);
          self->local_buffer_drain_level = start_position;

          /* If the Autostart parameter has been set to TRUE, don't wait
           * for a Start event.  */
          if (self->autostart)
            {
              self->started = TRUE;
              self->local_clock = 0;
              self->elapsed_time = 0;
            }
          /* Begin pushing data from our local buffer downstream using the 
           * source pad.  Unless we are autostarted, this task will send 
           * silence until we get a Start message.  */
          result =
            gst_pad_start_task (self->srcpad,
                                (GstTaskFunction)
                                gst_looper_push_data_downstream, self->srcpad,
                                NULL);
          self->src_pad_task_running = TRUE;
        }

      /* If the sink pad is in pull mode, it will have a task doing pulls.
       * That task will shut itself down when it notices that the local buffer
       * has been filled.  */

      /* Don't send the EOS event downstream until we are shut down.  */
      gst_event_unref (event);
      g_rec_mutex_unlock (&self->interlock);
      break;

    default:
      result = gst_pad_push_event (self->srcpad, event);
      break;
    }

  return result;
}

/* Handle an event from the source pad.  */
static gboolean
gst_looper_handle_src_event (GstPad * pad, GstObject * parent,
                             GstEvent * event)
{
  gboolean result = TRUE;
  GstLooper *self = GST_LOOPER (parent);
  const GstStructure *event_structure;
  const gchar *structure_name;
  guint64 start_position;

  GST_DEBUG_OBJECT (self, "received an event on the source pad.");
  g_rec_mutex_lock (&self->interlock);
  switch (GST_EVENT_TYPE (event))
    {
    case GST_EVENT_FLUSH_START:
      /* All the downstream data comes from here, so we block the event
       * from propagating upstream.  */
      gst_event_unref (event);
      /* if we are already sending our buffer downstream, stop.  */
      if (self->src_pad_task_running)
        {
          gst_pad_stop_task (self->srcpad);
          self->src_pad_task_running = FALSE;
        }
      result = TRUE;
      self->src_pad_flushing = TRUE;
      break;

    case GST_EVENT_FLUSH_STOP:
      /* All the downstream data comes from here, so we do not propagate
       * the event upstream.  */
      gst_event_unref (event);

      /* If the incoming buffer has been filled, start the task
       * which pushes data downstream.  */
      if ((self->data_buffered) && (!self->src_pad_task_running))
        {
          result =
            gst_pad_start_task (self->srcpad,
                                (GstTaskFunction)
                                gst_looper_push_data_downstream, self->srcpad,
                                NULL);
          self->src_pad_task_running = TRUE;
        }
      if (!result)
        {
          GST_DEBUG_OBJECT (self, "unable to start task on flush stop");
        }
      result = TRUE;
      self->src_pad_flushing = FALSE;
      break;

    case GST_EVENT_RECONFIGURE:
      /* FIXME: may have to start the loop task here.  */
      result = gst_pad_push_event (self->sinkpad, event);
      break;

    case GST_EVENT_CUSTOM_UPSTREAM:
      g_rec_mutex_lock (&self->interlock);

      /* We use five custom upstream events: start, pause, continue, release
       * and shutdown.
       * The release event is processed mostly in the envelope plugin,
       * but we also use it here to terminate looping.
       *
       * The start event causes the buffered data to be transmitted
       * from its beginning.
       *
       * The shutdown event is issued prior to closing down the
       * pipeline.  The looper sends EOS and stops sending data.
       *
       * The pause event silences the looper, and the continue event
       * lets it proceed from where it paused.  These are distinct from
       * the paused state of the pipeline because we want the pipeline
       * to keep running even if the looper is paused.  */
      event_structure = gst_event_get_structure (event);
      structure_name = gst_structure_get_name (event_structure);

      if (g_strcmp0 (structure_name, (gchar *) "start") == 0)
        {
          /* This is a start event, which might be caused by receipt
           * of a Note On MIDI message, or by an operator pushing a
           * start button.  Start pushing our local buffer downstream.
           */
          GST_INFO_OBJECT (self, "received custom start event");
          self->started = TRUE;
          self->completion_sent = FALSE;
          start_position = round_down_to_position (self, self->start_time);
          self->local_buffer_drain_level = start_position;
          self->elapsed_time = 0;
        }

      if (g_strcmp0 (structure_name, (gchar *) "pause") == 0)
        {
          /* The pause event can be caused by receipt of a command
           * to temporarily suspend sound output.  */
          GST_INFO_OBJECT (self, "received custom pause event");
          self->paused = TRUE;
          self->continued = FALSE;
        }

      if (g_strcmp0 (structure_name, (gchar *) "continue") == 0)
        {
          /* When the need for the pause has passed, another command
           * will resume the sound.  */
          GST_INFO_OBJECT (self, "received custom continue event");
          self->continued = TRUE;
        }

      if (g_strcmp0 (structure_name, (gchar *) "release") == 0)
        {
          /* The release event might be caused by receipt of a Note Off
           * MIDI message, or by an operator pushing a stop button.
           * Terminate any looping.  */
          GST_INFO_OBJECT (self, "received custom release event");
          self->released = TRUE;
        }

      if (g_strcmp0 (structure_name, (gchar *) "shutdown") == 0)
        {
          /* The shutdown event is caused by the operator shutting down
           * the application.  We send an EOS and stop.  */
          self->send_EOS = TRUE;
          GST_INFO_OBJECT (self, "shutting down");
        }

      g_rec_mutex_unlock (&self->interlock);

      /* Push the event upstream.  */
      result = gst_pad_push_event (self->sinkpad, event);
      break;

    default:
      result = gst_pad_push_event (self->sinkpad, event);
      break;
    }

  g_rec_mutex_unlock (&self->interlock);
  return result;
}

/* Handle a query on the element.  */
static gboolean
gst_looper_handle_query (GstElement * element, GstQuery * query)
{
  GstLooper *self = GST_LOOPER (element);

  /* Simply forward to the source pad query function.  */
  return gst_looper_handle_src_query (self->srcpad, GST_OBJECT_CAST (element),
                                      query);
}

/* Handle a query on the source pad or element.  */
static gboolean
gst_looper_handle_src_query (GstPad * pad, GstObject * parent,
                             GstQuery * query)
{
  GstLooper *self = GST_LOOPER (parent);
  GstFormat format;
  gint64 segment_start, segment_end;
  gboolean seekable, peer_success;
  gint64 peer_pos;
  GstSchedulingFlags scheduling_flags = 0;
  gboolean result;

  GST_DEBUG_OBJECT (self, "query on source pad or element");
  g_rec_mutex_lock (&self->interlock);

  switch (GST_QUERY_TYPE (query))
    {
    case GST_QUERY_POSITION:
      gst_query_parse_position (query, &format, &peer_pos);
      GST_DEBUG_OBJECT (self, "query position on source pad");

      peer_success = gst_pad_peer_query (self->sinkpad, query);
      switch (format)
        {
          /* FIXME: code this.  */
        case GST_FORMAT_BYTES:
          break;
        case GST_FORMAT_TIME:
          break;
        default:
          GST_DEBUG_OBJECT (self, "dropping query in %s format.",
                            gst_format_get_name (format));
          g_rec_mutex_unlock (&self->interlock);
          return FALSE;
        }
      result = TRUE;
      break;

    case GST_QUERY_DURATION:
      GST_DEBUG_OBJECT (self, "query duration on source pad");
      /* FIXME: code this.  */
      result = TRUE;
      break;

      /* FIXME: query buffering?  */

    case GST_QUERY_SCHEDULING:
      GST_DEBUG_OBJECT (self, "query scheduling on source pad");
      peer_success = gst_pad_peer_query (self->sinkpad, query);
      gst_query_parse_scheduling (query, &scheduling_flags, NULL, NULL, NULL);
      /* We only do push mode on our source pad.  */
      result = FALSE;
      break;

    case GST_QUERY_SEEKING:
      GST_DEBUG_OBJECT (self, "query seeking on source pad");
      peer_success = gst_pad_peer_query (self->sinkpad, query);
      gst_query_parse_seeking (query, &format, &seekable, &segment_start,
                               &segment_end);

      /* FIXME: since we buffer everything here, maybe do not propagate
       * the query but just answer it here.  */
      result = TRUE;
      break;

    case GST_QUERY_CAPS:
      /* The next element downstream wants to know what formats this pad
       * supports, and in what order of preference.  Just pass the query
       * upstream since we don't care.  */
      GST_DEBUG_OBJECT (self, "query caps on source pad");
      peer_success = gst_pad_query_default (pad, parent, query);
      GST_DEBUG_OBJECT (self, "completed query caps on source pad");
      result = peer_success;
      break;

    default:
      GST_DEBUG_OBJECT (self, "taking default action for query.");
      peer_success = gst_pad_query_default (pad, parent, query);
      if (!peer_success)
        {
          GST_DEBUG_OBJECT (self, "failed default query action");
        }
      result = peer_success;
    }

  GST_DEBUG_OBJECT (self, "completed source or element query processing.");
  g_rec_mutex_unlock (&self->interlock);
  return result;
}

/* Handle a query to the sink pad.  */
static gboolean
gst_looper_handle_sink_query (GstPad * pad, GstObject * parent,
                              GstQuery * query)
{
  GstLooper *self = GST_LOOPER (parent);
  gboolean result;

  GST_DEBUG_OBJECT (self, "received query on sink pad");
  g_rec_mutex_lock (&self->interlock);

  switch (GST_QUERY_TYPE (query))
    {
    case GST_QUERY_CAPS:
      /* The next element upstream is asking what formats this pad
       * supports, and in what order of preference.  Since we have
       * no preference ourselves, just pass the query downstream.  */
      GST_DEBUG_OBJECT (self, "query caps on sink pad");
      result = gst_pad_query_default (pad, parent, query);
      GST_DEBUG_OBJECT (self, "completed caps query on sink pad");
      break;

    default:
      result = gst_pad_query_default (pad, parent, query);
      break;
    }

  GST_DEBUG_OBJECT (self, "completed query on sink pad.");
  g_rec_mutex_unlock (&self->interlock);
  return result;
}

/* Round a time to the next higher buffer position from which we can start
 * a frame.  */
static guint64
round_up_to_position (GstLooper * self, guint64 specified_time)
{
  guint64 position;             /* unrounded buffer position corresponding to
                                 * the specified time.  */
  guint frame_size;             /* The number of bytes in one frame.  */
  guint64 frame_index;          /* The number of complete frames before the 
                                 * position.  */
  guint64 byte_position;        /* the position in the buffer corresponding to 
                                 * the beginning of the first frame after the 
                                 * specified time.  */

  /* The time is specified in nanoseconds.  If the buffer position 
   * corresponding to that time isn't on a frame boundary, convert it to the 
   * next higher buffer positon that is on a frame boundary.  */
  position = (gdouble) specified_time *self->bytes_per_ns;
  frame_size = self->width * self->channel_count / 8;
  frame_index = (position / frame_size);
  byte_position = frame_index * frame_size;
  if (byte_position < position)
    {
      byte_position = (frame_index + 1) * frame_size;
    }
  GST_DEBUG_OBJECT (self,
                    "time %" GST_TIME_FORMAT " rounded up to %"
                    GST_TIME_FORMAT " yielding buffer position %"
                    G_GUINT64_FORMAT ".", GST_TIME_ARGS (specified_time),
                    GST_TIME_ARGS (byte_position / self->bytes_per_ns),
                    byte_position);
  return byte_position;
}

/* Round a time to the next lower buffer position from which we can start
 * a frame.  */
static guint64
round_down_to_position (GstLooper * self, guint64 specified_time)
{
  guint64 position;             /* unrounded buffer position corresponding to
                                 * the specified time.  */
  guint frame_size;             /* The number of bytes in one frame.  */
  guint64 frame_index;          /* The number of complete frames before the 
                                 * position.  */
  guint64 byte_position;        /* the position in the buffer corresponding to 
                                 * the beginning of the first frame before the 
                                 * specified time.  */

  /* The time is specified in nanoseconds.  If the buffer position 
   * corresponding to that time isn't on a frame boundary, convert it to the 
   * next lower buffer positon that is on a frame boundary.  */
  position = (gdouble) specified_time *self->bytes_per_ns;
  frame_size = self->width * self->channel_count / 8;
  frame_index = (position / frame_size);
  byte_position = frame_index * frame_size;
  GST_DEBUG_OBJECT (self,
                    "time %" GST_TIME_FORMAT " rounded down to %"
                    GST_TIME_FORMAT " for buffer position %" G_GUINT64_FORMAT
                    ".", GST_TIME_ARGS (specified_time),
                    GST_TIME_ARGS (byte_position / self->bytes_per_ns),
                    byte_position);
  return byte_position;
}

/* Subroutine to read the data chunks from a WAV file into the local buffer.
 * This is a faster way to load the buffer than waiting for the data to
 * be provided in real time by upstream.  We read only the data; parsing of
 * the metadata is done by upstream.  The return value is TRUE if data was
 * read successfully, FALSE if not.  */
static gboolean
read_wav_file_data (GstLooper * self, guint64 max_position)
{
  FILE *file_stream;
  int stream_status;
  size_t amount_read;
  GstMemory *memory_allocated;
  GstMapInfo buffer_memory_info;
  int result, seek_success;
  gboolean return_value = FALSE;
  guint32 header[2];
  guint32 chunk_size;
  unsigned int byte_offset;
  guint64 local_buffer_fill_level;
  char data_byte;
  char *byte_data_out_pointer;

  /* This subroutine exits through some common cleanup code at common_exit.
   * The following flags control the extent of its cleanup.  */
  gboolean file_open = FALSE;
  gboolean buffer_mapped = FALSE;

  GST_DEBUG_OBJECT (self, "reading from wave file \"%s\".",
                    self->file_location);
  errno = 0;

  file_stream = fopen (self->file_location, "rb");
  if (file_stream == NULL)
    {
      GST_DEBUG_OBJECT (self, "failed to open file \"%s\": %s.",
                        self->file_location, strerror (errno));
      goto common_exit;
    }
  file_open = TRUE;

  /* Read the first eight bytes of the file, which is the RIFF header.  */
  amount_read = fread (&header, 1, 8, file_stream);
  if (amount_read != 8)
    {
      GST_DEBUG_OBJECT (self, "failed to read first 8 bytes: got %lu.",
                        amount_read);
      goto common_exit;
    }
  if (memcmp (&header[0], "RIFF", 4) != 0)
    {
      GST_DEBUG_OBJECT (self, "file \"%s\" is not a RIFF file.",
                        self->file_location);
      goto common_exit;
    }

  /* We don't care about the second word of the RIFF header, which is supposed
   * to be the size of the file but is inaccurate if the file is too large
   * for a 32-bit integer or was written in real time by a recording application
   * that did not know how long the data would be.  */

  /* Read and verify bytes 9 through 12 of the file.  */
  amount_read = fread (&header, 1, 4, file_stream);
  if (amount_read != 4)
    {
      GST_DEBUG_OBJECT (self, "failed to read bytes 9 through 12: got %lu.",
                        amount_read);
      goto common_exit;
    }
  if (memcmp (&header[0], "WAVE", 4) != 0)
    {
      GST_DEBUG_OBJECT (self, "file \"%s\" is not a WAVE file.",
                        self->file_location);
      goto common_exit;
    }

  /* Skip all but data chunks.  Copy the data from the data chunks into
   * our local buffer.  Since we are ignoring the size field of the RIFF
   * chunk, continue until end of file.  */
  local_buffer_fill_level = 0;
  while TRUE
    {
      /* If we have enough data to reach max duration, we don't need any more.
       * It doesn't hurt to read a little more data than is required, so
       * we need only check at chunk boundaries.  */
      if ((max_position != 0) && (local_buffer_fill_level > max_position))
        {
          GST_DEBUG_OBJECT (self,
                            "reached max duration at %" G_GUINT64_FORMAT ".",
                            max_position);
          break;
        }

      /* Read the first 8 bytes of the chunk to learn its type and size.  */
      amount_read = fread (&header, 1, 8, file_stream);
      if (amount_read != 8)
        {
          GST_DEBUG_OBJECT (self, "unable to read another eight bytes.");
          break;
        }

      chunk_size = header[1];
      if (memcmp (&header[0], "data", 4) != 0)
        {
          /* Skip over this non-data chunk.  Odd chunk sizes are padded with a
           * single byte so that chunks always start on 2-byte boundaries.  */
          if ((chunk_size & 1) == 1)
            chunk_size = chunk_size + 1;
          GST_DEBUG_OBJECT (self, "skipping forward by %u bytes.",
                            chunk_size);
          seek_success = fseek (file_stream, chunk_size, SEEK_CUR);
          if (seek_success != 0)
            {
              GST_DEBUG_OBJECT (self, "seek failed on file \"%s\": %d.",
                                self->file_location, seek_success);
              goto common_exit;
            }
          continue;
        }

      /* Copy the data chunk into our local buffer.  */
      GST_DEBUG_OBJECT (self, "reading %d bytes of data from file \"%s\".",
                        chunk_size, self->file_location);
      memory_allocated = gst_allocator_alloc (NULL, chunk_size, NULL);
      gst_buffer_append_memory (self->local_buffer, memory_allocated);
      result =
        gst_buffer_map (self->local_buffer, &buffer_memory_info,
                        GST_MAP_WRITE);
      if (!result)
        {
          GST_DEBUG_OBJECT (self, "unable to map local buffer for writing");
          goto common_exit;
        }
      buffer_mapped = TRUE;
      for (byte_offset = 0; byte_offset < chunk_size;
           byte_offset = byte_offset + 1)
        {
          byte_data_out_pointer =
            (void *) buffer_memory_info.data + local_buffer_fill_level +
            byte_offset;
          amount_read = fread (&data_byte, 1, 1, file_stream);
          if (amount_read != 1)
            {
              GST_DEBUG_OBJECT (self,
                                "failed to read a data byte from \"%s\".",
                                self->file_location);
              goto common_exit;
            }
          *byte_data_out_pointer = data_byte;
        }
      gst_buffer_unmap (self->local_buffer, &buffer_memory_info);
      buffer_mapped = FALSE;

      local_buffer_fill_level = local_buffer_fill_level + chunk_size;

      /* If the chunk size is odd, skip the pad byte.  */
      if ((chunk_size & 1) == 1)
        {
          amount_read = fread (&data_byte, 1, 1, file_stream);
          if (amount_read != 1)
            {
              GST_DEBUG_OBJECT (self,
                                "failed to read a pad byte from \"%s\".",
                                self->file_location);
              goto common_exit;
            }
        }
    }


  /* We failed to read a new chunk header.  Take this to mean end of file.  */
  self->local_buffer_fill_level = local_buffer_fill_level;
  GST_DEBUG_OBJECT (self, "Loaded %" G_GUINT64_FORMAT " bytes from file %s.",
                    local_buffer_fill_level, self->file_location);
  return_value = TRUE;

common_exit:
  if (buffer_mapped)
    {
      gst_buffer_unmap (self->local_buffer, &buffer_memory_info);
      buffer_mapped = FALSE;
    }

  if (file_open)
    {
      stream_status = fclose (file_stream);
      if (stream_status == EOF)
        {
          GST_DEBUG_OBJECT (self, "failed to close file \"%s\".",
                            self->file_location);
          return_value = FALSE;
        }
      file_open = FALSE;
    }

  return return_value;
}

/* Set the value of a property.  */
static void
gst_looper_set_property (GObject * object, guint prop_id,
                         const GValue * value, GParamSpec * pspec)
{
  GstLooper *self = GST_LOOPER (object);

  g_rec_mutex_lock (&self->interlock);

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
      GST_INFO_OBJECT (self, "loop-to: %" G_GUINT64_FORMAT ".",
                       self->loop_to);
      GST_OBJECT_UNLOCK (self);
      break;

    case PROP_LOOP_FROM:
      GST_OBJECT_LOCK (self);
      self->loop_from = g_value_get_uint64 (value);
      GST_INFO_OBJECT (self, "loop-from: %" G_GUINT64_FORMAT ".",
                       self->loop_from);
      GST_OBJECT_UNLOCK (self);
      break;

    case PROP_LOOP_LIMIT:
      GST_OBJECT_LOCK (self);
      self->loop_limit = g_value_get_uint (value);
      GST_INFO_OBJECT (self, "loop-limit: %" G_GUINT32_FORMAT ".",
                       self->loop_limit);
      GST_OBJECT_UNLOCK (self);
      break;

    case PROP_MAX_DURATION:
      GST_OBJECT_LOCK (self);
      self->max_duration = g_value_get_uint64 (value);
      GST_INFO_OBJECT (self, "max-duration: %" G_GUINT64_FORMAT ".",
                       self->max_duration);
      GST_OBJECT_UNLOCK (self);
      break;

    case PROP_START_TIME:
      GST_OBJECT_LOCK (self);
      self->start_time = g_value_get_uint64 (value);
      GST_INFO_OBJECT (self, "start-time: %" G_GUINT64_FORMAT ".",
                       self->start_time);
      GST_OBJECT_UNLOCK (self);
      break;

    case PROP_AUTOSTART:
      GST_OBJECT_LOCK (self);
      self->autostart = g_value_get_boolean (value);
      GST_INFO_OBJECT (self, "autostart: %d", self->autostart);
      GST_OBJECT_UNLOCK (self);
      break;

    case PROP_FILE_LOCATION:
      GST_OBJECT_LOCK (self);
      g_free (self->file_location);
      self->file_location = g_value_dup_string (value);
      GST_INFO_OBJECT (self, "file-location set to %s.", self->file_location);
      self->file_location_specified = TRUE;
      GST_OBJECT_UNLOCK (self);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
  g_rec_mutex_unlock (&self->interlock);
}

/* Return the value of a property.  */
static void
gst_looper_get_property (GObject * object, guint prop_id, GValue * value,
                         GParamSpec * pspec)
{
  GstLooper *self = GST_LOOPER (object);
  gchar *string_value;
  gdouble double_value;

  g_rec_mutex_lock (&self->interlock);
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

    case PROP_MAX_DURATION:
      GST_OBJECT_LOCK (self);
      g_value_set_uint64 (value, self->max_duration);
      GST_OBJECT_UNLOCK (self);
      break;

    case PROP_START_TIME:
      GST_OBJECT_LOCK (self);
      g_value_set_uint64 (value, self->start_time);
      GST_OBJECT_UNLOCK (self);
      break;

    case PROP_AUTOSTART:
      GST_OBJECT_LOCK (self);
      g_value_set_boolean (value, self->autostart);
      GST_OBJECT_UNLOCK (self);
      break;

    case PROP_FILE_LOCATION:
      GST_OBJECT_LOCK (self);
      g_value_set_string (value, self->file_location);
      GST_OBJECT_UNLOCK (self);
      break;

    case PROP_ELAPSED_TIME:
      GST_OBJECT_LOCK (self);
      double_value = (gdouble) self->elapsed_time / (gdouble) 1e9;
      string_value = g_strdup_printf ("%4.1f", double_value);
      g_value_set_string (value, string_value);
      g_free (string_value);
      string_value = NULL;
      GST_OBJECT_UNLOCK (self);
      break;

    case PROP_REMAINING_TIME:
      GST_OBJECT_LOCK (self);
      GST_OBJECT_UNLOCK (self);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
  g_rec_mutex_unlock (&self->interlock);

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
                   "Repeat a section of the input stream", looper_init,
                   VERSION, "LGPL", "GStreamer", "http://gstreamer.net/")
