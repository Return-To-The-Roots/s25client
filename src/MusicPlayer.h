// $Id: MusicPlayer.h 9357 2014-04-25 15:35:25Z FloSoft $
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
#ifndef MUSICPLAYER_H_INCLUDED
#define MUSICPLAYER_H_INCLUDED

#include "Singleton.h"

#pragma once

class iwMusicPlayer;

/// Speichert die Daten über eine Playlist und verwaltet diese
class Playlist
{
    public:
        Playlist();

        /// bereitet die Playlist aufs abspielen vor.
        void Prepare();

        /// liefert den Dateinamen des akteullen Songs
        const std::string getCurrentSong() const    {   return (songs.size() && order.size() ? songs[order[0]] : "");   }

        /// schaltet einen Song weiter und liefert den Dateinamen des aktuellen Songs
        const std::string getNextSong();

        /// Playlist in Datei speichern
        bool SaveAs(const std::string filename, const bool overwrite);
        /// Playlist laden
        bool Load(const std::string filename);

        /// Füllt das iwMusicPlayer-Fenster mit den entsprechenden Werten
        void FillMusicPlayer(iwMusicPlayer* window) const;
        /// Liest die Werte aus dem iwMusicPlayer-Fenster
        void ReadMusicPlayer(const iwMusicPlayer* const window);

        /// Wählt den Start-Song aus
        void SetStartSong(const unsigned id);

    protected:
        unsigned int repeats;               ///< Anzahl der Wiederholungen
        bool random;                        ///< Zufallswiedergabe?
        std::vector<std::string> songs;     ///< Dateinamen der abzuspielenden Titel
        std::vector<unsigned int> order;    ///< Reihenfolge der Titel
};

/// Globaler Musikplayer bzw. eine abspielbare Playlist
class MusicPlayer : public Singleton<MusicPlayer>
{
    public:
        MusicPlayer();

        /// Startet Abspielvorgang
        void Play();
        /// Stoppt Abspielvorgang
        void Stop();

        /// Playlist laden
        bool Load(const std::string filename) { return list.Load(filename); }
        /// Musik wurde fertiggespielt (Callback)
        void MusicFinished()    {   PlayNext(); }
        /// liefert die Playlist.
        Playlist& GetPlaylist() { return list;}

    protected:
        /// Spielt nächstes Stück ab
        void PlayNext();

    private:
        bool playing;                   ///< Läuft die Musik gerade?
        Playlist list;                  ///< Unsere aktuell aktive Playlist
        libsiedler2::ArchivInfo sng;    ///< externes benutzerdefiniertes Musikstück (z.B. andere mp3)
};

#endif // !MUSICPLAYER_H_INCLUDED
