# WORK IN PROGRESS, NOT RELEASED YET

The following is a draft of readme that will eventually be published, it may contain information that is not yet true but is expected to become true once the project is finished.

# Licar

*free and selfless KISS racing game*

![](media/logo.png)
![](media/ss2.png)
![](media/ss10.png)
![](https://static.wikitide.net/commonswiki/0/03/Licar_platforms.jpg)

For more detail see also the [game manual](media/manual.txt).

TODO: link to licar-content

## What Is This

This is a libre (free as in freedom) 3D racing game very much inspired by the proprietary game Trackmania (but different enough to be legally safe). It is written in a selfless way (it won't try to exploit you in any way, not even indirectly) following a philosophy of high minimalism, excluding any poison and bullshit of so called "modern technology" and self interest. It was made completely from scratch by a single man who is too different (for better or worse) from all other humans.

The game's objective is to finish given track as fast as possible. Unlike in typical racing games there is no live multiplayer: one doesn't race against other players directly, players instead compete in a speedrunning fashion, by individually trying to finish the same track faster, utilizing skill, tricks, shortcuts and potentially -- if they agree on it -- even computer assistance. Players don't even have to compete and may rather collaborate on finding faster solutions, analyzing tracks, looking for faster ways etc.

Licar isn't just a game meant for entertainment, it's also a kind of manifesto, expression of worldview, an engineering art, a tool (for education, research, ...) and possibly much more. It is NOT, on the other hand, a product, brand or franchise.

Considering just the player's perspective, some highlightable **features** of the game include:

- **replays**: small files that record individual runs merely as a sequence of inputs -- this is good for sharing runs, proving records, creating TAS runs etc.
- **custom maps**: players can create their own maps using a simple plaintext format.
- **ghosts**: players may race against ghost (collisionless) opponents.
- **great many settings**: basically everything is modifiable.
- **zero price, no ads, no subscriptions, amazing performance, low hardware demands, small size, no DRM** (and similar bullshit nowadays expected in every "modern" game).
- **REAL "retro" style**: not just mimicking the look and feel of an old game, but actually BEING what old games were.
- **absolute moddability**: absolutely EVERY single aspect of the game can be modified, there exist no "protections" or restriction, legal or technical.

Please note however the game is still relatively simple and so **the physics IS shitty at times**, just as it used to be in old games, there are no plans to make the physics flawless or more accurate, consider it a feature. If you'd like to integrate a more perfect physics engine in your custom fork, you're welcome to :)

**Cheating in this game is allowed.** We don't give a shit about drama, serious competition and totalitarian crap, just relax, drive and have fun, search for the best solution to each map by any means you deem entertaining.

## Why This Game Is Special

- **Everything is written in KISS/suckless C99 from scratch**. The whole project, INCLUDING the 3D physics engine, 3D rendering engine, game logic AND assets embedded in source code easily fit under **under 25 KLOC** in total, even when counting ALL lines of codes and mutually exclusive parts of code (different frontends).
- **Everything is created 100% from scratch** by the author, including code (of the game, physics engine, rendering engine, frontends), textures (created from own photos), 3D models, map, sound effects, music (created using custom sound font made from own recorded samples), font, accompanying materials etcetc. This ensures absolute freedom, there is no shadow of a doubt there is indeed is no 3rd party copyright. Everything was also **created exclusively with free software**.
- It has **custom KISS 3D physics engine** (tinyphysicsengine) written from scratch, implemented in a single header file, using **no GPU or floating point**.
- It has **custom KISS 3D software rendering engine** (small3dlib) written from scratch, implemented in a single header file, using **no GPU or floating point**.
- The whole game **doesn't use floating point at all**, everything is done with **32bit fixed point only**.
- It **doesn't have any dependency libraries, not even the C standard library** (except for using types from *stdint.h*, which can be trivially replaced with a few *defines*), everything is written from scratch for **maximum portability**. Libraries such as SDL are used for frontends, but they are never an inherent dependency and can be basically drop-in replaced with a similar library that's only required to be able to draw pixels and read keyboard. Almost everything can be turned off, the game can even **run without a file system** as the essential assets are embedded right in the source code -- files are only needed for optional extended functionality like saving replays, adding extra maps etc. Settings are part of the source code too, there are **no config files**.
- **No build system** is used, everything is in a **single compilation unit**, to build the game only a single compiler invocation is needed. This removes another bloat and dependency.
- The compiled game INCLUDING ASSETS is **extremely small** -- depending on platform, game and compilation settings, a playable executable can take only around 128 kB; in normal scenarios it will most likely fit under 400 kB.
- **Very fast and memory efficient**, the game will run practically on any computer save for the weakest of embedded devices. In general, despite being a fully 3D rendered game with 3D physics, the game can be run with **less than 500 KB RAM and 50 MHz CPU**; of course such hardware will require many features turned off and playability will suffer, but let's say 1 MB of RAM along with a 200 MHz CPU (laughable specs even 20 years ago) will be enough for comfortable play already.
- It is **exceptionally portable and future proof** thanks to having no dependencies besides C99 compiler. So far it was ported to and testplayed on:
  - **GNU/Linux**: via SDL2, CSFML or X11, also works through Wine
  - **Window$**: via SDL2
  - **OpenBSD**: via SDL2
  - **web browser** (even ran in ebook reader): via translation to JavaScript with emscripten
  - **Circuitmess Ringo AKA MAKERphone** (32b 160 MHz, 520 KB RAM)
  - **Raspberry Pi** (5, 3B+): with GNU/Linux
  - on resource-limited computers, in very limited ways (simplest rendering with low FPS, only loading tiniest maps, no ghosts, replays or audio):
    - **Pokitto** (32bit 48 MHz CPU, 36 KB RAM)
    - **Circuitmess Nibble** (32b 160 MHz CPU, 80 KB RAM)
    - **ESPboy** (32b 160 MHz CPU, 80 KB RAM)
  - tested compilers: gcc, clang, tcc (currently bugging a bit), emscripten
- It also goes without saying the physics is **100% deterministic**, it behaves the same on every platform, independently of rendering FPS. Besides others this allows for recording and playing back very small replays and creating press forward maps.
- It is **super moddable and hackable**, the code is relatively simple and **well commented**, written to encourage hacking. There are no obstacles such as DRM, anti-cheating, obfuscation etc. Cheating and hacking is allowed, do literally what you want.
- **No codes of censorship, flags, furry mascots or similar political bullshit.**
- **No "modern" bullshit such as OOP, Python, Docker, meme design patterns, scripting, sandboxes, "memory safety" and similar crap**.
- It is **absolutely and completely public domain free software with ZERO conditions on use** under CC0, it is legally more free that most "FOSS" software, there is no copyleft or credit requirement, you can do ABSOLUTELY anything you want with the project. This is a **selfless project aiming for no benefit of the creator** (include any non-financial benefit as well), this is made purely to bring more good to the world without gaining any advantage for self.
- This game has **PURE SOUL** and REAL oldschool graphics in the "PS1" style. No Unity or Unreal, no "retro" imitation shaders, no assets from a store. This isn't a wannabe retro 90s pretender, this IS a real 90s game.
- The game is **finished**, no need to update it ever.

## Manifesto

At this point I can't possibly summarize my views and life philosophy satisfyingly in a few paragraphs. If you're interested, you'll be able to discover a rabbithole around my stuff, but I can almost certainly say you won't like it, so think twice.

## Legal "Rights"

**tl;dr**: Everything in this repository is CC0 + a waiver of all rights, completely public domain as much as humanly possible, do absolutely anything you want.

I, Miloslav Číž (drummyfish), have created everything in this repository, including but not limited to code, graphics, sprites, palettes, fonts, sounds, music samples, music, storyline and texts, even the font in the video trailer and drum sound samples for the soundtrack, completely myself from scratch, using completely and exclusive free as in freedom software, without accepting any contributions, with the goal of creating a completely original art which is not a derivative work of anyone else's existing work, so that I could assure that by waiving my intellectual property rights the work will become completely public domain with as little doubt as posible.

This work's goal is to never be encumbered by any exclusive intellectual property rights, it is intended to always stay completely and forever in the public domain, available for any use whatsoever.

I therefore release everything in this repository under CC0 1.0 (public domain, https://creativecommons.org/publicdomain/zero/1.0/) + a waiver of all other IP rights (including patents), which is as follows:

Each contributor to this work agrees that they waive any exclusive rights, including but not limited to copyright, patents, trademark, trade dress, industrial design, plant varieties and trade secrets, to any and all ideas, concepts, processes, discoveries, improvements and inventions conceived, discovered, made, designed, researched or developed by the contributor either solely or jointly with others, which relate to this work or result from this work. Should any waiver of such right be judged legally invalid or ineffective under applicable law, the contributor hereby grants to each affected person a royalty-free, non transferable, non sublicensable, non exclusive, irrevocable and unconditional license to this right.

I would like to ask you, without it being any requirement at all, to please support free software and free culture by sharing at least some of your own work in a similar way I do with this project.

If you'd like to support me or just read something about me and my projects, visit my site: www.tastyfish.cz.
