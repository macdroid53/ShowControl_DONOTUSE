/* 
 * show_control, a GStreamer application.
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
#include <gst/base/gstbasetransform.h>
#include <gst/audio/audio.h>
#include <gst/audio/gstaudiofilter.h>

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
  GstAudioFilter element;

  /* Parameters */
  gboolean silent;
  guint64 loop_from;
  guint64 loop_to;
  guint loop_limit;

  /* Locals */
  guint64 loop_duration;
  guint64 timestamp_offset;
  guint loop_counter;
  guint channel_count;
};

struct _GstLooperClass
{
  GstAudioFilterClass parent_class;
};

GType gst_looper_get_type (void);

G_END_DECLS
#endif /* __GST_LOOPER_H__ */
