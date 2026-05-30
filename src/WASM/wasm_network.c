/* Emacs style mode select   -*- C++ -*-
 *-----------------------------------------------------------------------------
 *
 *  WASM multiplayer transport over WebSocket.
 *----------------------------------------------------------------------------*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_WEBSOCKET_NET

#include "SDL.h"

#include "protocol.h"
#include "i_network.h"
#include "i_system.h"
#include "wasm_io.h"

enum
{
  WASM_NET_DISCONNECTED = 0,
  WASM_NET_CONNECTING = 1,
  WASM_NET_OPEN = 2,
  WASM_NET_FAILED = 3
};

#define WASM_NET_POLL_MS 5

UDP_SOCKET udp_socket;
size_t sentbytes, recvdbytes;

static byte ChecksumPacket(const packet_header_t *buffer, size_t len)
{
  const byte *p = (const byte *)buffer;
  byte sum = 0;

  if (len == 0)
    return 0;

  while (p++, --len)
    sum += *p;

  return sum;
}

static void I_ShutdownNetwork(void)
{
  wasm_net_close();
  udp_socket = 0;
}

void I_InitNetwork(void)
{
  static int initialized;

  if (initialized)
    return;

  initialized = 1;
  I_AtExit(I_ShutdownNetwork, true);
}

UDP_SOCKET I_Socket(unsigned short port)
{
  (void)port;
  udp_socket = 1;
  return udp_socket;
}

int I_ConnectToServer(const char *serv)
{
  Uint32 started;

  if (wasm_net_connect(serv) < 0)
  {
    wasm_net_close();
    return -1;
  }

  started = SDL_GetTicks();
  while (1)
  {
    int state = wasm_net_state();
    Uint32 elapsed = SDL_GetTicks() - started;

    if (state == WASM_NET_OPEN)
      return 0;

    if (state == WASM_NET_DISCONNECTED || state == WASM_NET_FAILED)
    {
      wasm_net_close();
      return -1;
    }

    if (elapsed >= 5000)
    {
      wasm_net_close();
      return -1;
    }

    wasm_sleep((5000 - elapsed) < WASM_NET_POLL_MS ? (5000 - elapsed) : WASM_NET_POLL_MS);
  }
}

size_t I_GetPacket(packet_header_t *buffer, size_t buflen)
{
  int checksum;
  int len;

  len = wasm_net_receive(buffer, (int)buflen);
  if (len <= 0)
    return 0;

  checksum = buffer->checksum;
  buffer->checksum = 0;
  if (ChecksumPacket(buffer, (size_t)len) != checksum)
    return 0;

  recvdbytes += (size_t)len;
  return (size_t)len;
}

void I_SendPacket(packet_header_t *packet, size_t len)
{
  packet->checksum = ChecksumPacket(packet, len);
  if (wasm_net_send(packet, (int)len) > 0)
    sentbytes += len;
}

void I_WaitForPacket(int ms)
{
  Uint32 started = SDL_GetTicks();

  while (wasm_net_packet_len() == 0)
  {
    int state = wasm_net_state();
    Uint32 elapsed = SDL_GetTicks() - started;
    Uint32 remaining = WASM_NET_POLL_MS;

    if (ms >= 0 && elapsed >= (Uint32)ms)
      break;

    if (ms >= 0)
      remaining = (Uint32)ms - elapsed;

    if (state == WASM_NET_DISCONNECTED || state == WASM_NET_FAILED)
      break;

    wasm_sleep(remaining < WASM_NET_POLL_MS ? remaining : WASM_NET_POLL_MS);
  }
}

#endif
