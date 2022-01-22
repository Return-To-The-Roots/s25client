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

BOOST_AUTO_TEST_CASE(EmptySongsCauseException)
{
    const auto songs0 = std::vector<std::string>{"", "s02", "s03"};
    const auto songs1 = std::vector<std::string>{"s01", "", "s03"};
    const auto songs2 = std::vector<std::string>{"s01", "s02", ""};
    BOOST_REQUIRE_THROW(Playlist _(songs0, 0, false), std::invalid_argument);
    BOOST_REQUIRE_THROW(Playlist _(songs1, 0, false), std::invalid_argument);
    BOOST_REQUIRE_THROW(Playlist _(songs2, 0, false), std::invalid_argument);
    // However no songs are allowed and simply do nothing
    Playlist pl(std::vector<std::string>{}, rttr::test::randomValue(0u, 10u), rttr::test::randomBool());
    BOOST_TEST(pl.getSongs().empty());
    BOOST_TEST(pl.getCurrentSong().empty());
    BOOST_TEST(pl.getNextSong().empty());
    // Even when called multiple times
    BOOST_TEST(pl.getNextSong().empty());
    BOOST_TEST(pl.getNextSong().empty());
}

BOOST_AUTO_TEST_CASE(PlaylistPlaysInOrder)
{
    const auto songs = std::vector<std::string>{"s01", "s02", "s03"};
    Playlist pl(songs, 0, false);
    BOOST_TEST(pl.getCurrentSong().empty()); // Nothing yet
    for(int numPlaylistRepeats = rttr::test::randomValue(1, 5); numPlaylistRepeats > 0; --numPlaylistRepeats)
    {
        for(const auto& song : songs)
        {
            BOOST_TEST(pl.getNextSong() == song);
            BOOST_TEST(pl.getCurrentSong() == song);
        }
        // End of playlist, the next loop will repeat the same songs
    }
}

BOOST_AUTO_TEST_CASE(RepeatedPlaylistPlaysInOrder)
{
    const auto songs = std::vector<std::string>{"s01", "s02", "s03"};
    const auto numRepeats = rttr::test::randomValue(1u, 3u);
    Playlist pl(songs, numRepeats, false);
    BOOST_TEST(pl.getCurrentSong().empty()); // Nothing yet
    for(int numPlaylistRepeats = rttr::test::randomValue(1, 5); numPlaylistRepeats > 0; --numPlaylistRepeats)
    {
        for(const auto& song : songs)
        {
            // Note the `<=`: Repeats is the amount of repeats, i.e. 0==play once, no repeats
            for(unsigned i = 0; i <= numRepeats; i++)
            {
                BOOST_TEST(pl.getNextSong() == song);
                BOOST_TEST(pl.getCurrentSong() == song);
            }
        }
        // End of playlist, the next loop will repeat the same songs
    }
}

BOOST_AUTO_TEST_CASE(RandomPlaylistPlaysEachSongOncePerPlaylistRepeat)
{
    const auto songs = std::vector<std::string>{"s01", "s02", "s03"};
    Playlist pl(songs, 0, true);
    BOOST_TEST(pl.getCurrentSong().empty()); // Nothing yet
    for(int numPlaylistRepeats = rttr::test::randomValue(1, 5); numPlaylistRepeats > 0; --numPlaylistRepeats)
    {
        std::vector<std::string> playedSongs;
        for(unsigned i = 0; i < songs.size(); i++)
        {
            const auto& song = pl.getNextSong();
            BOOST_TEST(!song.empty());
            BOOST_TEST(pl.getCurrentSong() == song);
            playedSongs.push_back(song);
        }
        // End of playlist
        for(const auto& song : songs)
        {
            BOOST_TEST(helpers::contains(playedSongs, song));
        }
    }
}

BOOST_AUTO_TEST_CASE(RandomPlaylistPlaysEachSongNumRepeatsTimes)
{
    const auto songs = std::vector<std::string>{"s01", "s02", "s03"};
    const auto numRepeats = rttr::test::randomValue(2u, 5u);
    Playlist pl(songs, numRepeats, true);
    BOOST_TEST(pl.getCurrentSong().empty()); // Nothing yet
    for(int numPlaylistRepeats = rttr::test::randomValue(1, 5); numPlaylistRepeats > 0; --numPlaylistRepeats)
    {
        std::vector<std::string> playedSongs;
        for(unsigned i = 0; i < songs.size() * (numRepeats + 1u); i++)
        {
            const auto& song = pl.getNextSong();
            BOOST_TEST(!song.empty());
            BOOST_TEST(pl.getCurrentSong() == song);
            playedSongs.push_back(song);
        }
        // End of playlist
        for(const auto& song : songs)
        {
            const unsigned numPlayed = helpers::count(playedSongs, song);
            BOOST_TEST(numPlayed == numRepeats + 1u);
        }
    }
}

BOOST_AUTO_TEST_CASE(SaveLoadResultsInSamePlaylist)
{
    const auto songs = std::vector<std::string>{"folder/song.ogg", "song with space.ogg", "windows_folder\\song.mp3"};
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
    // Playlist can be played
    const auto& firstSong = pl.getNextSong();
    BOOST_TEST_REQUIRE(!firstSong.empty());
    BOOST_TEST(helpers::contains(songs, firstSong));
}

BOOST_AUTO_TEST_SUITE_END()
