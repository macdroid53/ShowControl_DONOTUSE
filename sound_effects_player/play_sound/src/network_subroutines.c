/*
 * network_subroutines.c
 *
 * Copyright © 2015 by John Sauter <John_Sauter@systemeyescomputerstore.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
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

#include <gtk/gtk.h>
#include <gio/gio.h>
#include "play_sound.h"
#include "parse_subroutines.h"
#include "network_subroutines.h"

/* The persistent data used by the network subroutines. */

struct network_info
{
  gchar *network_buffer;
  gint port_number;
  GSocketService *service;
};

/* Subroutines to handle network messages */

/* Receive incoming data. */
static void
receive_data_callback (GObject * source_object, GAsyncResult * result,
                       gpointer user_data)
{
  struct network_info *network_data;
  gchar *network_buffer;
  GInputStream *istream;
  GError *error = NULL;
  gssize nread;

  istream = G_INPUT_STREAM (source_object);

  nread = g_input_stream_read_finish (istream, result, &error);
  if (error || nread <= 0)
    {
      if (error)
        g_error_free (error);
      g_input_stream_close (istream, NULL, NULL);
    }
  if (nread != 0)
    {
      network_data = play_sound_get_network_data (user_data);
      network_buffer = network_data->network_buffer;
      network_buffer[nread] = '\0';
      /* Because network data is streamed, data may be received in
       * arbitrary-sized chunks.  Processing a chunk might range from
       * just adding it to a buffer to executing several commands that
       * arrived all at once. */
      parse_text (network_buffer, user_data);

      /* Continue reading. */
      g_input_stream_read_async (istream, network_buffer, network_buffer_size,
                                 G_PRIORITY_DEFAULT, NULL,
                                 receive_data_callback, user_data);
    }

  return;
}

/* Receive an incoming connection. */
static gboolean
incoming_callback (GSocketService * service, GSocketConnection * connection,
                   GObject * source_object, gpointer user_data)
{
  GInputStream *istream;
  struct network_info *network_data;
  gchar *network_buffer;

  istream = g_io_stream_get_input_stream (G_IO_STREAM (connection));
  network_data = play_sound_get_network_data (user_data);
  network_buffer = network_data->network_buffer;
  /* Start reading from the connection. */
  g_input_stream_read_async (istream, network_buffer, network_buffer_size,
                             G_PRIORITY_DEFAULT, NULL, receive_data_callback,
                             user_data);
  return FALSE;
}

/* Initialize the network subroutines.  We start to listen for messages.
 * The return value is the persistent data. */
void *
network_init (GApplication * app)
{
  GError *error = NULL;
  GSocketService *service;
  gchar *network_buffer;
  struct network_info *network_data;

  /* Allocate the persistent information. */
  network_data = g_malloc (sizeof (struct network_info));

  /* Allocate the network buffer. */
  network_buffer = g_malloc0 (network_buffer_size);
  network_data->network_buffer = network_buffer;

  /* Set the default port. */
  network_data->port_number = 1500;

  /* Create a new socket service. */
  service = g_socket_service_new ();
  network_data->service = service;

  /* Connect to the port. */
  g_socket_listener_add_inet_port ((GSocketListener *) service,
                                   network_data->port_number, G_OBJECT (app),
                                   &error);
  /* Check for errors. */
  if (error != NULL)
    {
      g_error (error->message);
    }

  /* Listen for the "incoming" signal, which says that we have a connection. */
  g_signal_connect (service, "incoming", G_CALLBACK (incoming_callback), app);

  /* Start the socket service. */
  g_socket_service_start (service);

  return network_data;
}

/* Set the network port number. */
void
network_set_port (int port_number, GApplication * app)
{
  GError *error = NULL;
  GSocketService *service;
  struct network_info *network_data;

  network_data = play_sound_get_network_data (app);
  network_data->port_number = port_number;
  g_print ("Network port set to %i.\n", port_number);

  /* Stop network processing on the old port. */
  service = network_data->service;
  g_socket_service_stop (service);
  g_socket_listener_close ((GSocketListener *) service);
  g_object_unref (service);

  /* Create a new service to listen on the new port. */
  service = g_socket_service_new ();
  network_data->service = service;

  /* Connect to the new port. */
  g_socket_listener_add_inet_port ((GSocketListener *) service,
                                   network_data->port_number, G_OBJECT (app),
                                   &error);
  /* Check for errors. */
  if (error != NULL)
    {
      g_error (error->message);
    }

  /* Listen for the "incoming" signal, which says that we have a connection. */
  g_signal_connect (service, "incoming", G_CALLBACK (incoming_callback), app);

  /* Start the socket service. */
  g_socket_service_start (service);

  return;
}
