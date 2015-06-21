__author__ = 'mac'
import socket

class UDPClient(object):

  def __init__(self, address, port):
    """Initialize the client.

    As this is UDP it will not actually make any attempt to connect to the
    given server at ip:port until the send() method is called.
    """
    self._sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    self._sock.setblocking(0)
    self._address = address
    self._port = port

  def send(self, content):
    """Sends to the server."""
    self._sock.sendto(bytes(content, 'UTF-8'), (self._address, self._port))

