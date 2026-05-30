/* Emacs style mode select   -*- C++ -*-
 *-----------------------------------------------------------------------------
 *
 *
 *  PrBoom: a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2000 by
 *  Jess Haas, Nicolas Kalkhof, Colin Phipps, Florian Schulze
 *  Copyright 2005, 2006 by
 *  Florian Schulze, Colin Phipps, Neil Stevens, Andrey Budko
 *  Copyright (C) 2024 by
 *  Gregory Maynard-Hoare
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 *  02111-1307, USA.
 *
 * DESCRIPTION:
 *   WASM IO support
 *
 *-----------------------------------------------------------------------------
 */

#include "wasm_io.h"

#include <emscripten.h>

enum
{
  WASM_NET_DISCONNECTED = 0,
  WASM_NET_CONNECTING = 1,
  WASM_NET_OPEN = 2,
  WASM_NET_FAILED = 3
};

EM_JS(int, wasm_net_js_connect, (const char *endpoint), {
  const MAX_QUEUED_BYTES = 1024 * 1024;
  const MAX_PACKET_BYTES = 64 * 1024;
  if (!Module.dwasmNet) {
    Module.dwasmNet = {
      socket: null,
      queue: [],
      queuedBytes: 0,
      state: 0
    };
  }

  const net = Module.dwasmNet;
  if (typeof net.queuedBytes !== 'number') {
    net.queuedBytes = 0;
  }
  const failSocket = (socket, reason, detail) => {
    if (net.socket !== socket) {
      return;
    }

    const messageDetail = (detail === undefined || detail === null) ? "" : detail;
    console.warn(reason, messageDetail);
    net.socket = null;
    net.state = 3;
    if (socket.readyState < WebSocket.CLOSING) {
      socket.close();
    }
  };
  const enqueuePacket = (socket, packet) => {
    if (net.socket !== socket) {
      return;
    }

    if (packet.length > MAX_PACKET_BYTES) {
      failSocket(socket, 'Multiplayer relay packet exceeded limit:', packet.length);
      return;
    }

    // Packet counts climb quickly when a browser window is unfocused. Keep a
    // byte cap so memory stays bounded, but do not tear the socket down just
    // because many small packets arrived while the client was throttled.
    if (net.queuedBytes + packet.length > MAX_QUEUED_BYTES) {
      failSocket(socket, 'Multiplayer relay receive queue exceeded limit:', net.queuedBytes + packet.length);
      return;
    }

    net.queue.push(packet);
    net.queuedBytes += packet.length;
  };
  const address = UTF8ToString(endpoint);
  const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
  let url = address;

  if (new RegExp('^[a-zA-Z][a-zA-Z0-9+.-]*://').test(url)) {
  } else if (url.startsWith('//')) {
    url = protocol + url;
  } else {
    url = protocol + '//' + url;
  }

  if (net.socket) {
    const staleSocket = net.socket;
    net.socket = null;
    staleSocket.close();
  }

  net.queue = [];
  net.queuedBytes = 0;
  net.state = 1;

  try {
    const socket = new WebSocket(url);
    net.socket = socket;
    socket.binaryType = 'arraybuffer';
    socket.onopen = () => {
      if (net.socket !== socket) {
        return;
      }

      net.state = 2;
      console.info('Connected multiplayer relay:', url);
    };
    socket.onmessage = (event) => {
      if (net.socket !== socket) {
        return;
      }

      if (event.data instanceof ArrayBuffer) {
        enqueuePacket(socket, new Uint8Array(event.data));
      } else if (ArrayBuffer.isView(event.data)) {
        enqueuePacket(socket, new Uint8Array(event.data.buffer.slice(event.data.byteOffset, event.data.byteOffset + event.data.byteLength)));
      } else if (event.data && typeof event.data.arrayBuffer === 'function') {
        event.data.arrayBuffer().then((buffer) => {
          if (net.socket !== socket) {
            return;
          }

          enqueuePacket(socket, new Uint8Array(buffer));
        });
      } else {
        console.warn('Ignoring non-binary multiplayer frame.');
      }
    };
    socket.onerror = () => {
      if (net.socket !== socket) {
        return;
      }

      if (net.state !== 2) {
        net.state = 3;
      }
      console.warn('Multiplayer relay socket error.');
    };
    socket.onclose = () => {
      if (net.socket !== socket) {
        return;
      }

      net.socket = null;
      net.state = 3;
      console.info('Multiplayer relay disconnected.');
    };
  } catch (error) {
    net.socket = null;
    net.queue = [];
    net.state = 3;
    console.warn('Failed to create multiplayer relay socket:', error);
    return -1;
  }

  return 0;
});

EM_JS(int, wasm_net_js_state, (), {
  return Module.dwasmNet ? Module.dwasmNet.state : 0;
});

EM_JS(int, wasm_net_js_send, (const void *data, int len), {
  const MAX_BUFFERED_BYTES = 1024 * 1024;
  const net = Module.dwasmNet;
  if (!net || !net.socket || net.state !== 2) {
    return -1;
  }

  if (net.socket.readyState !== WebSocket.OPEN) {
    const socket = net.socket;
    net.socket = null;
    net.state = 3;
    if (socket && socket.readyState < WebSocket.CLOSING) {
      socket.close();
    }
    return -1;
  }

  if (net.socket.bufferedAmount > MAX_BUFFERED_BYTES) {
    const socket = net.socket;
    console.warn('Multiplayer relay send backlog exceeded limit:', net.socket.bufferedAmount);
    net.socket = null;
    net.state = 3;
    socket.close();
    return -1;
  }

  try {
    net.socket.send(HEAPU8.slice(data, data + len));
    return len;
  } catch (error) {
    const socket = net.socket;
    console.warn('Multiplayer relay send failed:', error);
    if (socket && socket.readyState === WebSocket.OPEN) {
      net.state = 3;
      net.socket = null;
      socket.close();
    } else {
      net.socket = null;
      net.state = 0;
    }
    return -1;
  }
});

EM_JS(int, wasm_net_js_packet_len, (), {
  const net = Module.dwasmNet;
  if (!net || !net.queue.length) {
    return 0;
  }

  return net.queue[0].length;
});

EM_JS(int, wasm_net_js_receive, (void *buffer, int buflen), {
  const net = Module.dwasmNet;
  if (!net || !net.queue.length) {
    return 0;
  }

  const packet = net.queue.shift();
  net.queuedBytes -= packet.length;
  const len = Math.min(packet.length, buflen);
  HEAPU8.set(packet.subarray(0, len), buffer);
  return len;
});

EM_JS(void, wasm_net_js_close, (), {
  const net = Module.dwasmNet;
  if (!net || !net.socket) {
    if (net) {
      net.queue = [];
      net.queuedBytes = 0;
      net.state = 0;
    }
    return;
  }

  const socket = net.socket;
  net.socket = null;
  net.queue = [];
  net.queuedBytes = 0;
  net.state = 0;
  socket.close();
});

static int soft_exit_code;

void wasm_init_fs(void)
{
  // Fetch from IDBFS in the background.  This should only be called once!
  EM_ASM(
    Module.save_counter = 0;
    Module.restore_busy = 1;
    FS.mkdir("/dwasm");
    FS.mount(IDBFS, {}, "/dwasm");
    console.info("Loading data...");
    FS.syncfs(true, function(err) {
      if (err)
        console.warn("Failed to load data:", err);
      else
        console.info("Data loaded.");

      Module.restore_busy = 0;
    });
  );
}

int wasm_restore_busy(void)
{
  // Verify whether IDBFS restore is complete
  return EM_ASM_INT(
    return Module.restore_busy;
  );
}

void wasm_sync_fs(void)
{
  // Sync to IDBFS in the background
  EM_ASM(
    Module.save_counter++;
    console.info("Saving data...");
    FS.syncfs(function(err) {
      Module.save_counter--;

      if (err)
        console.warn("Failed to save data:", err);
      else
        console.info("Data saved.");
    });
  );
}

void wasm_hide_console(void)
{
  // Hide the game console and show the canvas
  EM_ASM(
    if (typeof Module.hideConsole === 'function')
      Module.hideConsole();
  );
}

void wasm_show_console(void)
{
  // Show the game console and hide the canvas
  EM_ASM(
    if (typeof Module.showConsole === 'function')
      Module.showConsole();
  );
}

void wasm_vid_resize(void)
{
  // Notify JS after a resolution change
  EM_ASM(
    if (typeof Module.winResized === 'function')
      Module.winResized();
  );
}

void wasm_capture_mouse(void)
{
  // Ensure the pointer is captured in the canvas
  EM_ASM(
    if (typeof Module.captureMouse === 'function')
      Module.captureMouse();
  );
}

void wasm_sleep(unsigned int ms)
{
  emscripten_sleep((int)ms);
}

int wasm_net_connect(const char *endpoint)
{
  return wasm_net_js_connect(endpoint);
}

int wasm_net_state(void)
{
  return wasm_net_js_state();
}

int wasm_net_send(const void *data, int len)
{
  return wasm_net_js_send(data, len);
}

int wasm_net_packet_len(void)
{
  return wasm_net_js_packet_len();
}

int wasm_net_receive(void *buffer, int buflen)
{
  return wasm_net_js_receive(buffer, buflen);
}

void wasm_net_close(void)
{
  wasm_net_js_close();
}

void wasm_soft_exit(int exit_code)
{
  soft_exit_code = exit_code;
  emscripten_set_main_loop(wasm_soft_exit_fs_check, 0, 0);
}

void wasm_soft_exit_fs_check(void)
{
  // Called repeatedly on program exit until everything is saved
  if (!EM_ASM_INT(
    return Module.save_counter;
  )) {
    emscripten_cancel_main_loop();
    EM_ASM({
      if (typeof Module.softExit === 'function')
        Module.softExit($0);
    }, soft_exit_code);
  }
}
