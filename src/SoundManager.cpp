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
#include "defines.h"
#include "SoundManager.h"

#include "Loader.h"
#include "drivers/VideoDriverWrapper.h"
#include "drivers/AudioDriverWrapper.h"
#include "Settings.h"
#include "nodeObjs/noBase.h"
#include "GameClient.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

SoundManager::SoundManager() : last_bird(0), bird_interval(0), ocean_play_id(0)
{
}

SoundManager::~SoundManager()
{}

void SoundManager::PlayNOSound(const unsigned sound_lst_id, noBase* const obj, const unsigned int id, unsigned char volume)
{
    if (GAMECLIENT.IsPaused())
        return;

    if(!SETTINGS.sound.effekte)
        return;

    // Wird Sound schon gespielt?
    for(std::list<NOSound>::iterator it = no_sounds.begin(); it != no_sounds.end(); ++it)
    {
        if(it->obj == obj && it->id == id)
            return;
    }

    // Sound wird noch nicht gespielt --> hinzufügen und abspielen
    unsigned play_id = LOADER.GetSoundN("sound", sound_lst_id)->Play(volume, false);

    // Konnte er auch abgespielt werden?

    if(play_id != 0)
    {
        // Dann hinzufügen zur abgespielt-Liste
        NOSound nos = { obj, id, play_id };
        no_sounds.push_back(nos);
    }

}

void SoundManager::WorkingFinished(noBase* const obj)
{
    if (GAMECLIENT.IsPaused())
        return;

    if(SETTINGS.sound.effekte == false)
        return;
    // Alle Sounds von diesem Objekt stoppen und löschen
    for(std::list<NOSound>::iterator it = no_sounds.begin(); it != no_sounds.end();)
    {
        if(it->obj == obj)
        {
            AUDIODRIVER.StopEffect(it->play_id);
            it = no_sounds.erase(it);
        }else
            ++it;
    }
}


void SoundManager::PlayBirdSounds(const unsigned short tree_count)
{
    if (GAMECLIENT.IsPaused())
        return;

    if(!SETTINGS.sound.effekte)
        return;

    // Abstände zwischen den Vogelsounds berechnen (je nachdem wieviel Bäume)
    unsigned interval;
    if(1000 > tree_count * 10)
        interval = 1000 - tree_count * 10;
    else
        interval = 200;

    interval += bird_interval;

    // Nach einiger Zeit neuen Sound abspielen
    if(VIDEODRIVER.GetTickCount() - last_bird  > interval)
    {
        // ohne baum - kein vogel
        if(tree_count > 0)
            LOADER.GetSoundN("sound", 87 + rand() % 5)->Play(80 - rand() % 30, false);
        last_bird = VIDEODRIVER.GetTickCount();
        bird_interval = rand() % 1000;
    }
}

void SoundManager::PlayOceanBrawling(const unsigned water_percent)
{
    if (GAMECLIENT.IsPaused())
        return;

    if(!SETTINGS.sound.effekte)
        return;

    // Ist genug Wasser da zum Rauschen?
    if(water_percent > 10)
    {
        // Wird schon ein Sound gespielt?
        if(!AUDIODRIVER.IsEffectPlaying(ocean_play_id))
        {
            // Wenn nicht --> neuen abspielen
            ocean_play_id = LOADER.GetSoundN("sound", 98 + rand() % 3)->Play(255, true);
        }

        // Lautstärke setzen
        AUDIODRIVER.ChangeVolume(ocean_play_id, water_percent * 2 + 55);
    }
    else
    {
        // Rauschen ggf. stoppen
        if(ocean_play_id)
            AUDIODRIVER.StopEffect(ocean_play_id);
    }
}

void SoundManager::StopAll()
{
    if(ocean_play_id)
        AUDIODRIVER.StopEffect(ocean_play_id);

    last_bird = VIDEODRIVER.GetTickCount();
    bird_interval = 0;
}

