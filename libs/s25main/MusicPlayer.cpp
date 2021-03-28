// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
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

#include "MusicPlayer.h"
#include "Loader.h"
#include "drivers/AudioDriverWrapper.h"
#include "ogl/MusicItem.h"
#include "libsiedler2/Archiv.h"
#include "libsiedler2/ArchivItem.h"
#include "libsiedler2/ErrorCodes.h"
#include "libsiedler2/prototypen.h"
#include "s25util/Log.h"
#include "s25util/StringConversion.h"

MusicPlayer::MusicPlayer() : playing(false) {}

/**
 *  Startet Abspielvorgang
 */
void MusicPlayer::Play()
{
    playing = true;
    PlayNext();
}

/**
 *  Stoppt Abspielvorgang
 */
void MusicPlayer::Stop()
{
    AUDIODRIVER.StopMusic();
    playing = false;
}

/**
 * Spielt nächstes Stück ab
 */
void MusicPlayer::PlayNext()
{
    std::string song = list.getNextSong();

    // Am Ende der Liste angekommen?
    if(song.empty())
    {
        Stop();
        return;
    }

    // Evtl ein Siedlerstück ("sNN")?
    if(song.length() == 3 && song[0] == 's')
    {
        unsigned nr = s25util::fromStringClassicDef(song.substr(1), 999u);
        if(nr > 0 && nr <= LOADER.sng_lst.size())
            LOADER.sng_lst[nr - 1]->Play();
        return;
    }

    // anderes benutzerdefiniertes Stück abspielen
    // in "sng" speichern, daher evtl. altes Stück erstmal löschen
    sng.clear();

    LOG.write(_("Loading \"%1%\": ")) % song;

    // Neues Stück laden
    if(int ec = libsiedler2::loader::LoadSND(song, sng))
    {
        LOG.write(_("Error: %1%\n")) % libsiedler2::getErrorString(ec);
        Stop();
        return;
    }
    LOG.write(_("OK\n"));

    // Und abspielen
    auto* curSong = dynamic_cast<MusicItem*>(sng[0]);
    if(curSong)
        curSong->Play(1);
}
