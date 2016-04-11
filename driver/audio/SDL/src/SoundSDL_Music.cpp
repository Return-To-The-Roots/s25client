// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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
#include "main.h" // IWYU pragma: keep
#include "SoundSDL_Music.h"

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

SoundSDL_Music::SoundSDL_Music() : music(NULL)
{
}

SoundSDL_Music::~SoundSDL_Music()
{
    Mix_FreeMusic(music);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Abspielen Starten.
 *
 *  @param[in] loop  @p true für ununterbrochenes Abspielen
 *
 *  @return @p true bei Erfolg, @p false bei Fehler
 *
 *  @author FloSoft
 */
//int SoundSDL_Music::Play(unsigned char volume, const unsigned char volume, const bool loop)
//{
//  // ggf alten anhalten
//  Stop(-1);
//
//  if(Mix_PlayMusic(music, (loop ? -1 : 0)) == -1)
//  {
//      fprintf(stderr, "%s\n", Mix_GetError());
//      return -1;
//  }
//
//  mthis = this;
//
//  // Volume nur von 0-128
//  Mix_VolumeMusic(volume / 2);
//  Mix_HookMusicFinished(MusicFinished);
//
//  return 0;
//}
//


///////////////////////////////////////////////////////////////////////////////
/**
 *  Abspielen Stoppen.
 *
 *  @param[in] channel Ignored
 *
 *  @author FloSoft
 */
//void SoundSDL_Music::Stop(int channel)
//{
//  Mix_HaltMusic();
//}
