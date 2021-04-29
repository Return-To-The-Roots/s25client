// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
