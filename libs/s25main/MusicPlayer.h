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
#ifndef MUSICPLAYER_H_INCLUDED
#define MUSICPLAYER_H_INCLUDED

#include "Playlist.h"
#include "libsiedler2/Archiv.h"
#include "s25util/Singleton.h"
#include <string>
#include <vector>

/// Globaler Musikplayer bzw. eine abspielbare Playlist
class MusicPlayer : public Singleton<MusicPlayer, SingletonPolicies::WithLongevity>
{
public:
    static constexpr unsigned Longevity = 31; // After AudioDriverWrapper

    MusicPlayer();

    /// Startet Abspielvorgang
    void Play();
    /// Stoppt Abspielvorgang
    void Stop();

    /// Musik wurde fertiggespielt (Callback)
    void MusicFinished() { PlayNext(); }
    /// liefert die Playlist.
    const Playlist& GetPlaylist() const { return list; }
    void SetPlaylist(Playlist pl) { list = std::move(pl); }

protected:
    /// Spielt nächstes Stück ab
    void PlayNext();

private:
    bool playing;            /// Läuft die Musik gerade?
    Playlist list;           /// Unsere aktuell aktive Playlist
    libsiedler2::Archiv sng; /// externes benutzerdefiniertes Musikstück (z.B. andere mp3)
};

#define MUSICPLAYER MusicPlayer::inst()

#endif // !MUSICPLAYER_H_INCLUDED
