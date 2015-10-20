/* 
 * File: gstenvelope.h, part of show_control, a GStreamer application.
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

#ifndef __GST_ENVELOPE_H__
#define __GST_ENVELOPE_H__

#include <gst/gst.h>
#include <gst/base/gstbasetransform.h>
#include <gst/audio/audio.h>
#include <gst/audio/gstaudiofilter.h>

G_BEGIN_DECLS
#define GST_TYPE_ENVELOPE \
  (gst_envelope_get_type())
#define GST_ENVELOPE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_ENVELOPE,GstEnvelope))
#define GST_ENVELOPE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_ENVELOPE,GstEnvelopeClass))
#define GST_IS_ENVELOPE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_ENVELOPE))
#define GST_IS_ENVELOPE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_ENVELOPE))
typedef struct _GstEnvelope GstEnvelope;
typedef struct _GstEnvelopeClass GstEnvelopeClass;

struct _GstEnvelope
{
  GstAudioFilter element;

  /* Parameters */
  gboolean silent;
  GstClockTimeDiff attack_duration_time;
  gdouble attack_level;
  GstClockTimeDiff decay_duration_time;
  gdouble sustain_level;
  GstClockTimeDiff release_start_time;
  gchar *release_duration_string;
  gdouble volume;
  gboolean autostart;
  gchar *sound_name;

  /* Locals */
  GstClockTimeDiff release_duration_time;
  gboolean release_duration_infinite;
  gboolean release_started;
  gdouble release_started_volume;
  gdouble last_volume;
  GstClockTimeDiff release_started_time;
  gchar *last_message;
  gboolean external_release_seen;
  gboolean external_completion_seen;
  gboolean application_notified;
  gboolean completed;
  gboolean running;
  gboolean started;
  gboolean pause_seen;
  gboolean continue_seen;
  gboolean pausing;
  GstClockTime base_time;
  GstClockTimeDiff pause_time;
  GstClockTime pause_start_time;
};

struct _GstEnvelopeClass
{
  GstAudioFilterClass parent_class;
};

GType gst_envelope_get_type (void);

G_END_DECLS
#endif /* __GST_ENVELOPE_H__ */
