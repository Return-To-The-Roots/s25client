// Copyright (c) 2016 - 2020 Settlers Freaks (sf-team at siedler25.org)
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

#include "Playlist.h"
#include "helpers/containerUtils.h"
#include "rttr/test/TmpFolder.hpp"
#include "rttr/test/random.hpp"
#include "s25util/Log.h"
#include <boost/test/unit_test.hpp>
#include <algorithm>

BOOST_AUTO_TEST_SUITE(PlaylistTests)

BOOST_AUTO_TEST_CASE(DefaultConstructedPlaylistIsEmpty)
{
    Playlist pl;
    BOOST_TEST(pl.getSongs().empty());
    BOOST_TEST(pl.getCurrentSong().empty());
    BOOST_TEST(pl.getNextSong().empty());
    pl.SetStartSong(0); // No error
    BOOST_TEST(pl.getNextSong().empty());
}

BOOST_AUTO_TEST_CASE(PlaylistPlaysInOrder)
{
    const auto songs = std::vector<std::string>{"s01", "s02", "s03"};
    Playlist pl(songs, 1, false);
    BOOST_TEST(pl.getCurrentSong().empty()); // Nothing yet
    for(const auto& song : songs)
    {
        BOOST_TEST(pl.getNextSong() == song);
        BOOST_TEST(pl.getCurrentSong() == song);
    }
    // End of playlist
    BOOST_TEST(pl.getNextSong().empty());
    BOOST_TEST(pl.getCurrentSong().empty());
}

BOOST_AUTO_TEST_CASE(RepeatedPlaylistPlaysInOrder)
{
    const auto songs = std::vector<std::string>{"s01", "s02", "s03"};
    const auto numRepeats = rttr::test::randomValue(2u, 5u);
    Playlist pl(songs, numRepeats, false);
    BOOST_TEST(pl.getCurrentSong().empty()); // Nothing yet
    for(unsigned i = 0; i < numRepeats; i++)
    {
        for(const auto& song : songs)
        {
            BOOST_TEST(pl.getNextSong() == song);
            BOOST_TEST(pl.getCurrentSong() == song);
        }
    }
    // End of playlist
    BOOST_TEST(pl.getNextSong().empty());
    BOOST_TEST(pl.getCurrentSong().empty());
}

BOOST_AUTO_TEST_CASE(RandomPlaylistPlaysEachSongOnce)
{
    const auto songs = std::vector<std::string>{"s01", "s02", "s03"};
    Playlist pl(songs, 1, true);
    BOOST_TEST(pl.getCurrentSong().empty()); // Nothing yet
    std::vector<std::string> playedSongs;
    for(unsigned i = 0; i < songs.size(); i++)
    {
        const auto song = pl.getNextSong();
        BOOST_TEST(!song.empty());
        BOOST_TEST(pl.getCurrentSong() == song);
        playedSongs.push_back(song);
    }
    // End of playlist
    BOOST_TEST(pl.getNextSong().empty());
    BOOST_TEST(pl.getCurrentSong().empty());
    for(const auto& song : songs)
    {
        BOOST_TEST(helpers::contains(playedSongs, song));
    }
}

BOOST_AUTO_TEST_CASE(RandomPlaylistPlaysEachSongNumRepeatsTimes)
{
    const auto songs = std::vector<std::string>{"s01", "s02", "s03"};
    const auto numRepeats = rttr::test::randomValue(2u, 5u);
    Playlist pl(songs, numRepeats, true);
    BOOST_TEST(pl.getCurrentSong().empty()); // Nothing yet
    std::vector<std::string> playedSongs;
    for(unsigned i = 0; i < songs.size() * numRepeats; i++)
    {
        const auto song = pl.getNextSong();
        BOOST_TEST(!song.empty());
        BOOST_TEST(pl.getCurrentSong() == song);
        playedSongs.push_back(song);
    }
    // End of playlist
    BOOST_TEST(pl.getNextSong().empty());
    BOOST_TEST(pl.getCurrentSong().empty());
    for(const auto& song : songs)
    {
        const unsigned numPlayed = helpers::count(playedSongs, song);
        BOOST_TEST(numPlayed == numRepeats);
    }
}

BOOST_AUTO_TEST_CASE(SaveLoadResultsInSamePlaylist)
{
    const auto songs = std::vector<std::string>{"folder/s01.ogg", "s02 with space", "winfolder\\s03.mp3"};
    const auto numRepeats = rttr::test::randomValue(2u, 5u);
    const bool isRandom = rttr::test::randomValue(0u, 1u) == 1u;
    Playlist pl(songs, numRepeats, isRandom);
    rttr::test::TmpFolder tmpFolder;
    const auto filepath = tmpFolder.get() / "playlist.pll";
    BOOST_TEST_REQUIRE(pl.SaveAs(filepath));
    Playlist pl2;
    BOOST_TEST_REQUIRE(pl2.Load(LOG, filepath));
    BOOST_TEST(pl.getSongs() == pl2.getSongs(), boost::test_tools::per_element());
    BOOST_TEST(pl.getNumRepeats() == pl2.getNumRepeats());
    BOOST_TEST(pl.isRandomized() == pl2.isRandomized());
    // Playlist is prepared
    BOOST_TEST(!pl.getNextSong().empty());
}

BOOST_AUTO_TEST_SUITE_END()
