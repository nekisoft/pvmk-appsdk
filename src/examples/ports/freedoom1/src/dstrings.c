// Modified by Bryan E. Topp, 2021
//
// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
// $Log:$
//
// DESCRIPTION:
//	Globally defined strings.
// 
//-----------------------------------------------------------------------------

#ifdef __GNUG__
#pragma implementation "dstrings.h"
#endif
#include "dstrings.h"



char* endmsg[NUM_QUITMESSAGES+1]=
{
  // DOOM1
  QUITMSG,
  "please don't leave, there's more\ndemons to toast!",
  "let's beat it -- this is turning\ninto a bloodbath!",
  "i wouldn't leave if i were you.\nthe system menu is much worse.",
  "you're trying to say you like the\nsystem menu better than me, right?",
  "don't leave yet -- there's a\ndemon around that corner!",
  "ya know, next time you come in here\ni'm gonna toast ya.",
  "go ahead and leave. see if i care.",

  // QuitDOOM II messages
  "you want to quit?\nthen, thou hast lost an eighth!",
  "don't go now, there's a \ndimensional shambler waiting\nat the system menu!",
  "get outta here and go back\nto your boring programs.",
  "if i were your boss, i'd \n deathmatch ya in a minute!",
  "look, bud. you leave now\nand you forfeit your body count!",
  "just leave. when you come\nback, i'll be waiting with a bat.",
  "you're lucky i don't smack\nyou for thinking about leaving.",

  // FinalDOOM?
  "leaving so soon? fine, be that way.", //"fuck you, pussy!\nget the fuck out!",
  "i bet you're just giving up, loser.", //"you quit and i'll jizz\nin your cystholes!",
  "mad cuz bad\npress y to ragequit", //"if you leave, i'll make\nthe lord drink my jizz.",
  "not like the world needs saving anyway.", //"hey, ron! can we say\n'fuck' in the game?",
  "didn't you already play this\n30 years ago anyway?",//"i'd leave: this is just\nmore monsters and levels.\nwhat a load.",
  "have you noticed we gave you\n40 extra video lines free?", //"suck it down, asshole!\nyou're a fucking wimp!",
  "warning:\nplayer experiencing skill issues", //"don't quit now! we're \nstill spending your money!",

  // Internal debug. Different style, too.
  "THIS IS NO MESSAGE!\nPage intentionally left blank.",
};


  


