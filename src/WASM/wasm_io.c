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

void wasm_init_fs(void)
{
  // Fetch from IDBFS in the background
  EM_ASM(
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
    console.info("Saving data...");
    FS.syncfs(function(err) {
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
