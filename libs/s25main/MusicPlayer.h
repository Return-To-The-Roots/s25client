// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

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
