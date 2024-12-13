Dwasm
=====

This is an unofficial WebAssembly port of the engine behind the famous 3D first-person shooter *DOOM*.

[Run Dwasm in your browser here](https://gmh-code.github.io/dwasm/).

Dwasm is based upon *PrBoom+*, and inherits significant enhancements to the original engine.  Some notable features include:

- Widescreen aspect ratios
- Custom resolutions above the standard 320x200 (or 640x400)
- Frame rates above 35FPS
- Texture upscaling
- Realtime MIDI music synthesis through *Timidity* or *OPL2* (similar to *Sound Blaster* audio)
- Several heads-up display alternatives
- Crosshairs that indicate the locked target
- Statistic and timer overlays
- Backwards compatibility with several other engines
- WebGL acceleration (work in progress)

Also available are the following engines, which run very well in modern web browsers:

- [Qwasm](https://gmh-code.github.io/qwasm/) ([source](https://github.com/GMH-Code/Qwasm) based on id's original *Quake* engine code)
- [Qwasm2](https://gmh-code.github.io/qwasm2/) ([source](https://github.com/GMH-Code/Qwasm2) based on *Yamagi Quake II*)

IWAD Files
----------

If you want to build this project, you need a base `.wad` file to run.  IWADs contain maps, textures, sprites, sounds, and more.

### Compatible Games

[Freedoom](https://freedoom.github.io/) provides a recommended choice of two IWADs.  Either of these should be drop-in compatible.

The following releases should also work, but please only use legal copies and ensure you comply with the licences:

- DOOM (Shareware)
- DOOM (Full Version)
- DOOM 2: Hell on Earth
- The Ultimate DOOM
- Final DOOM - TNT: Evilution
- Final DOOM - The Plutonia Experiment
- HACX - Twitch 'n Kill
- Chex(R) Quest (+ Chex(R) Quest 2 with the correct PWAD)

In addition, any 'total conversion' mods based on these variants should run.

PWAD Files
----------

Patch WADS (still with the `.wad` extension) can be used in addition to IWAD files.  Resources such as maps will replace those in the original IWAD.  If you have multiple PWADS (patches-on-patches), the order in which you specify them can matter.  Consult the mod's documentation for more details.

DEH / BEX Files
---------------

The `.deh` (*DeHackEd*) format was written to change game elements by patching the original *DOOM* executable.  This project reads and applies patch modifications at runtime instead of producing a new binary.

The `.bex` format extends on `.deh` and is specific to engines based on *Boom*.  These should also work.

Playing the Shareware Version of DOOM
-------------------------------------

The shareware IWAD can be extracted from the original DOS archive, widely available online:

    Filename: doom19s.zip
    File size: 2,450,688 bytes (2.33 MiB)
    SHA1: 8D0FBBBEBA5ECB692A99F97E55DFB5365CFE5B77
    SHA256: CACF0142B31CA1AF00796B4A0339E07992AC5F21BC3F81E7532FE1B5E1B486E6

Inside are two files, `DOOMS_19.1` and `DOOMS_19.2`.  These files are in a split LZH-compressed format and should be joined together in order.  You can do this with several tools, but a quicker way is simply to use the `cat` command on Unix-based systems or `type` on Windows.

The resultant joined file can then be opened by using software like *7-Zip* or *WinRAR*, and contains:

    Filename: DOOM1.WAD
    File size: 4,196,020 bytes (4.00 MiB)
    SHA1: 5B2E249B9C5133EC987B3EA77596381DC0D6BC1D
    SHA256: 1D7D43BE501E67D927E415E0B8F3E29C3BF33075E859721816F652A526CAC771

### Licence Warnings

The licence for the shareware version of DOOM only appears to permit duplication of the archive that was originally obtained from an official source, so it appears as though, understandably, the archive's contents cannot be distributed nor embedded separately.

Do not be tempted to host commercial titles on a public server without permission from the copyright holder.  Licence compliance is entirely your responsibility.

In-Browser Saving
-----------------

Attempts will be made to commit data to browser storage when:

- Saving your game
- Changing settings and exiting the game

In this version, you get a unique collection of save points per IWAD, so you shouldn't run out if you play more than one game.

Most settings have an immediate effect, but some cannot be done in realtime and require a page reload.  Alterations should persist, providing the browser stores the data.

Command-Line Arguments
----------------------

Like the desktop version of PrBoom+, you can pass arguments to this version at runtime.  By default, the query portion of the URL is used, but the JavaScript code can be modified to use anything else, such as an input text box.

### Example

Let's say you were using the default template, serving the page locally, and you wanted to:

1. Pick the hardest skill level.
2. Make the monsters react twice as fast (good luck)!
3. Start the game on map `e1m2`.

To do this, you can start the game with `-s 5 -f -warp 1 2`.

In Dwasm, you can append a single `?` to the end of the URL and place `&` between each parameter and value, where you would usually put a space.  This would look something like:

    https://127.0.0.1/?-s&5&-f&-warp&1&2

Networking Support
------------------

WebSockets support for multiplayer has not yet been added.

It should be possible to connect to a WebSockets proxy to enable online play, but Dwasm will need rebuilding with the appropriate proxy configuration.

Cheat Codes
-----------

The original cheat codes can be entered during play and work as expected.

Known Issues / Workarounds
--------------------------

### OpenGL Renderer

The OpenGL mode did not exist in the original engine.  It is not the default renderer for these reasons:

- Frame rate can vary significantly due to the vastly different methods used for drawing the scene.
- Floor textures occasionally corrupt depending on settings and the location and angle of the player.  This may be a bug in GL4ES, and a workaround may be found in the future.  No options have yet been found in the engine or the transition layer that fix this issue.
- You cannot switch to OpenGL mode from software rendering, nor can you change the resolution in OpenGL mode, without exiting via the menu and then reloading the page.

### Software Rendering

The software renderer does not appear to exhibit any issues, but there is one note:

- Colour depths of under 32-bit will not run noticeably faster.  This is because the output format has to be converted to 32-bit anyway for the image to be displayed properly on the browser canvas.

### Melt Transition

The fullscreen 'melt' transition used between some events is skipped in this version, as is the delay that comes with it.

It is possible to enable this transition in the code, however, it either requires changes to allow the animation to control the main loop temporarily, or alternatively, a quicker way is to use Emscripten's `ASYNCIFY` feature along with other related options to limit its scope.  Generally using this is not recommended due to code/stack instrumentation bloat, slowdown, and loss of vertical synchronisation with the graphics card, meaning the actual game will usually run less smoothly overall.

How to Build on Linux for WebAssembly
-------------------------------------

### EMSDK

This is required to build both GL4ES and Dwasm.

Clone or download EMSDK: https://emscripten.org/docs/getting_started/downloads.html

Install and fully activate the latest version, as per the instructions.

### GL4ES (Optional)

Include GL4ES as part of the build if you want to enable the hardware graphics rendering support.  If the game is started up in software mode anyway, GL4ES will not be initialised nor used.

To enable this feature:

1. Clone or download GL4ES from https://ptitseb.github.io/gl4es/
2. Run the GL4ES *Emscripten* build according to the instructions on https://ptitseb.github.io/gl4es/COMPILE.html

### PrBoom+ WAD

This file is mandatory, but is usually generated as a one-off and is done when building the native version of PrBoom+ (which this project also supports).  To build the native version, run these commands in the Dwasm folder:

    mkdir build_native
    cd build_native
    cmake .. -DCMAKE_BUILD_TYPE=Release

The compatible file should be in the `build_native` folder and match this exactly:

    Filename: prboom-plus.wad
    File size: 357,768 bytes (349 KiB)
    SHA1: 8A799C6615702AAC72016B784E30EA1ABD3E197B
    SHA256: 29E6E18023CF39319F9984107F5F0F796B2593B1C3A55FC79ADF3AE12498A6CE

### Dwasm

To build the main project, place `prboom-plus.wad` and other files (such as an IWAD) that you would like to include into the `wasm/fs` folder.  All filenames must be in **lowercase**.

Next, run these commands in the Dwasm folder.  Replace `/tmp/gl4es` with your GL4ES build, if applicable.  If you decided not to include WebGL support, *completely* remove the option `-DGL4ES_PATH=/tmp/gl4es`.

    mkdir build
    cd build
    emcmake cmake .. -DCMAKE_BUILD_TYPE=Release -DGL4ES_PATH=/tmp/gl4es
    make

The process will output the following into the `build` folder:

    index.html
    index.js
    index.data
    index.wasm

These files can then be placed on a web server.  To reduce bandwidth and download time, compress all the files using GZip (or better, Brotli) compression, host the files statically, and verify the web browser is doing the decompression for each file.

A GZ-compressed build should be as little as (approximately) 1 megabyte, not including resources you add.

-----

This project is not affiliated with, endorsed by, or in any way connected to id Software, Bethesda Softworks, or ZeniMax Media.  All trademarks and copyrights are the property of their respective owners.
