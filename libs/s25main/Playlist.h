// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <boost/filesystem/path.hpp>
#include <string>
#include <vector>

#pragma once

class Log;

/// List of songs with meta data.
/// Also controls which song is played (next)
/// After all songs have been played the playlist is restarted (reshuffled if needed)
class Playlist
{
public:
    Playlist() = default;
    Playlist(std::vector<std::string> songs, unsigned numRepeats, bool random);

    /// Get the currently played song, or an empty string if none
    const std::string& getCurrentSong() const { return currentSong_; };

    /// Switches to the next song and returns its name
    const std::string& getNextSong();

    /// Save playlist to file
    bool SaveAs(const boost::filesystem::path& filepath) const;
    /// Load playlist from file
    bool Load(Log& logger, const boost::filesystem::path& filepath);

    const auto& getSongs() const { return songs_; }
    unsigned getNumRepeats() const { return numRepeats_; }
    bool isRandomized() const { return random_; }

    /// Moves the song with the given index (in the songs_ array) to the front of the list of songs to be played
    /// Has no effect if the song is not in the queue (anymore), e.g. if it was already played
    void SetStartSong(unsigned id);

private:
    /// Get the playlist ready for playing
    void Prepare();

    std::vector<std::string> songs_; /// Filenames of titles to play
    unsigned numRepeats_ = 1;        /// How often to repeat each song
    bool random_ = false;            /// True for random order, else in-order
    std::vector<unsigned> order_;    /// Actual order of the songs, indices into songs
    std::string currentSong_;
};
