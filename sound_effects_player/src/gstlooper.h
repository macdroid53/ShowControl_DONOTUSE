/* 
 * gstlooper.h, a file in sound_effects_player, a component of show_control, 
 * which is a GStreamer application.
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
 * or write to:
 * Free Software Foundation, Inc.
 * 51 Franklin Street, Fifth Floor
 * Boston, MA 02111-1301
 * USA.
 */

#ifndef __GST_LOOPER_H__
#define __GST_LOOPER_H__

#include <gst/gst.h>

G_BEGIN_DECLS
#define GST_TYPE_LOOPER \
  (gst_looper_get_type())
#define GST_LOOPER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_LOOPER,GstLooper))
#define GST_LOOPER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_LOOPER,GstLooperClass))
#define GST_IS_LOOPER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_LOOPER))
#define GST_IS_LOOPER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_LOOPER))
typedef struct _GstLooper GstLooper;
typedef struct _GstLooperClass GstLooperClass;

struct _GstLooper
{
  GstElement element;


  /* Parameters */
  gboolean silent;
  guint64 loop_from;
  guint64 loop_to;
  guint64 max_duration;
  guint64 start_time;
  gchar *file_location;
  guint loop_limit;
  gboolean autostart;

  /* Locals */

  GstPad *sinkpad;
  GstPad *srcpad;
  GstBuffer *local_buffer;      /* The buffer that holds the data to send 
                                 * downstream.  */

  guint64 local_buffer_fill_level;
  guint64 local_buffer_drain_level;
  guint64 local_buffer_size;    /* number of bytes in the local buffer */
  guint64 pull_level;           /* how much data we have pulled from upstream */
  guint64 timestamp_offset;
  guint64 local_clock;          /* The current time, in nanoseconds.  
                                 * This counts continuously through loops.  */
  gdouble bytes_per_ns;         /* data rate in bytes per nanosecond */
  gchar *format;                /* The format of incoming data--for example,
                                 * F32LE.  */
  GRecMutex interlock;          /* used to prevent interference between tasks */
  guint64 loop_counter;
  guint64 width;                /* the size of a sample in bits */
  guint64 channel_count;        /* The number of channels of sound.  
                                 * Stereo has two.  A frame consists of
                                 * channel_count samples, each of width bits. */
  guint64 data_rate;            /* the data rate, in frames per second.  */
  GstPadMode src_pad_mode;      /* The mode of the source pad: push or pull. */
  GstPadMode sink_pad_mode;     /* The mode of the sink pad: push or pull.  */
  gboolean started;             /* We have received a Start signal.  */
  gboolean completion_sent;     /* We have sent a "complete" message downstream
                                 * to tell the envelope plugin that the
                                 * sound is complete.  */
  gboolean paused;              /* We have received a Pause signal, and it has 
                                 * not yet been canceled by a Continue signal.
                                 */
  gboolean continued;           /* We have received a Continue signal.  If both
                                 * paused and continued are set, we will resume
                                 * sending sound, and clear both.  */
  gboolean released;            /* We have received a Release signal.  */
  gboolean data_buffered;       /* We have received all the data we need into 
                                 * our sink pad.  */
  gboolean src_pad_active;      /* The source pad is active.  */
  gboolean sink_pad_active;     /* The sink pad is active.  */
  gboolean sink_pad_flushing;   /* The sink pad is flushing.  */
  gboolean src_pad_flushing;    /* The source pad is flushing.  */
  gboolean src_pad_task_running;        /* The task which pushes data downstream
                                         * on the source pad is active.  */
  gboolean sink_pad_task_running;       /* The task which pulls data from 
                                         * upstream on the sink pad is active.  
                                         */
  gboolean send_EOS;            /* The main task wants the pushing downstream 
                                 * task to send an end-of-stream message and 
                                 * terminate.  */
  gboolean state_change_pending;        /* The main task wants the pushing 
                                         * downstream task to complete a state 
                                         * change.  */
  gboolean file_location_specified;     /* The location of the wave file that
                                         * heads this bin has been specified.  
                                         */
  gboolean seen_incoming_data;  /* Sound data has been seen on the source pad.  
                                 */
  guint8 silence_byte;          /* The byte value of silence for this format.
                                 */
};

/* The number of bytes of data requested from upstream in each pull */
#define BUFFER_SIZE 4096

struct _GstLooperClass
{
  GstElementClass parent_class;

};

GType gst_looper_get_type (void);

G_END_DECLS
#endif /* __GST_LOOPER_H__ */
