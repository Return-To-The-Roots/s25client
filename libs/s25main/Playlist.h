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
#ifndef PLAYLIST_H_INCLUDED
#define PLAYLIST_H_INCLUDED

#include <boost/filesystem/path.hpp>
#include <string>
#include <vector>

#pragma once

class Log;

/// Speichert die Daten über eine Playlist und verwaltet diese
class Playlist
{
public:
    Playlist() = default;
    Playlist(std::vector<std::string> songs, unsigned numRepeats, bool random);

    /// liefert den Dateinamen des aktuellen Songs
    std::string getCurrentSong() const;

    /// schaltet einen Song weiter und liefert den Dateinamen des aktuellen Songs
    std::string getNextSong();

    /// Playlist in Datei speichern
    bool SaveAs(const boost::filesystem::path& filepath, bool overwrite) const;
    /// Playlist laden
    bool Load(Log& logger, const boost::filesystem::path& filepath);

    const auto& getSongs() const { return songs_; }
    unsigned getNumRepeats() const { return numRepeats_; }
    bool isRandomized() const { return random_; }

    /// Wählt den Start-Song aus
    void SetStartSong(unsigned id);

private:
    /// Get the playlist ready for playing
    void Prepare();

    std::vector<std::string> songs_; /// Filenames of titles to play
    unsigned numRepeats_ = 1;        /// How often to repeat all songs
    bool random_ = false;            /// True for random order, else in-order
    std::vector<unsigned> order_;    /// Actual order of the songs, indices into songs
    std::string currentSong_;
};

#endif // !PLAYLIST_H_INCLUDED
