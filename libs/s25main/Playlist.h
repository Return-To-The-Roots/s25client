// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

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
    bool SaveAs(const boost::filesystem::path& filepath) const;
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
