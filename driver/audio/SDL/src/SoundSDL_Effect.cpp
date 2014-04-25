// $Id: SoundSDL_Effect.cpp 9357 2014-04-25 15:35:25Z FloSoft $
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Return To The Roots.
//
// Return To The Roots is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Return To The Roots is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Return To The Roots. If not, see <http://www.gnu.org/licenses/>.

///////////////////////////////////////////////////////////////////////////////
// Header
#include "main.h"
#include <AudioDriver.h>
#include "SoundSDL_Effect.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////
/**
 *  Konstruktor von @p SoundSDL_Effect.
 *
 *  @author FloSoft
 */
SoundSDL_Effect::SoundSDL_Effect() : sound(NULL)
{
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Destruktor von @p SoundSDL_Effect.
 *
 *  @author FloSoft
 */
SoundSDL_Effect::~SoundSDL_Effect(void)
{
    if(sound)
        Mix_FreeChunk(sound);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Abspielen Starten.
 *
 *  @param[in] volume Lautstärke (0-254) für den Sound
 *  @param[in] loop   @p true für ununterbrochenes Abspielen
 *
 *  @return @p Play-ID bei Erfolg, @p 0xFFFFFFFF bei Fehler
 *
 *  @author FloSoft
 */
//int SoundSDL_Effect::Play(unsigned char volume, const unsigned char volume, const bool loop)
//{
//  int channel = Mix_PlayChannel(-1, sound, (loop ? -1 : 0));
//
//  if(channel == -1)
//  {
//      fprintf(stderr, "%s\n", Mix_GetError());
//      return 0xFFFFFFFF;
//  }
//
//  Mix_SetPanning(channel, volume, volume);
//
//  return channel;
//}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Abspielen Stoppen.
 *
 *  @param[in] channel Channel der gestoppt werden soll (-1 für alle)
 *
 *  @author FloSoft
 */
//void SoundSDL_Effect::Stop(int channel)
//{
//  Mix_HaltChannel(channel);
//}
