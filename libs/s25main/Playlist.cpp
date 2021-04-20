// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Playlist.h"
#include "mygettext/mygettext.h"
#include "s25util/Log.h"
#include "s25util/StringConversion.h"
#include <boost/algorithm/string/trim.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/nowide/fstream.hpp>
#include <algorithm>
#include <random>
#include <sstream>

namespace bnw = boost::nowide;

Playlist::Playlist(std::vector<std::string> songs, unsigned numRepeats, bool random)
    : songs_(std::move(songs)), numRepeats_(numRepeats), random_(random)
{
    Prepare();
}

/**
 *  startet das Abspielen der Playlist.
 */
void Playlist::Prepare()
{
    currentSong_.clear();
    // Add one entry per song repeated if required
    order_.resize(songs_.size() * numRepeats_);

    for(unsigned i = 0; i < songs_.size() * numRepeats_; ++i)
        order_[i] = i % songs_.size();

    // Shuffle if requested
    if(random_)
        std::shuffle(order_.begin(), order_.end(), std::mt19937(std::random_device()()));
}

std::string Playlist::getCurrentSong() const
{
    return currentSong_;
}

/**
 *  Playlist in Datei speichern
 */
bool Playlist::SaveAs(const boost::filesystem::path& filepath) const
{
    s25util::ClassicImbuedStream<bnw::ofstream> out(filepath);
    if(!out.good())
        return false;

    out << numRepeats_ << " ";
    out << (random_ ? "random" : "ordered") << std::endl;

    // songs reinschreiben
    for(const auto& song : songs_)
        out << song << "\n";

    out.close();

    return true;
}

/**
 *  Playlist laden
 */
bool Playlist::Load(Log& logger, const boost::filesystem::path& filepath)
{
    songs_.clear();
    if(filepath.empty())
        return false;

    logger.write(_("Loading %1%\n")) % filepath;

    bnw::ifstream in(filepath);

    if(in.fail())
        return false;

    std::string line, random_str;
    s25util::ClassicImbuedStream<std::stringstream> sline;

    if(!std::getline(in, line))
        return false;
    sline.clear();
    sline << line;
    if(!(sline >> numRepeats_ >> random_str))
        return false;

    random_ = (random_str == "random_playback" || random_str == "random");

    while(std::getline(in, line))
    {
        boost::algorithm::trim_if(line, [](char c) { return c == '\r' || c == '\n'; });
        if(line.empty())
            break;
        songs_.push_back(line);
    }

    if(in.bad())
        return false;

    // geladen, also zum Abspielen vorbereiten
    Prepare();
    return true;
}

// Wählt den Start-Song aus
void Playlist::SetStartSong(const unsigned id)
{
    for(auto& i : order_)
    {
        if(i == id)
        {
            std::swap(order_.front(), i);
            return;
        }
    }
}

/// schaltet einen Song weiter und liefert den Dateinamen des aktuellen Songs
std::string Playlist::getNextSong()
{
    if(order_.empty())
        currentSong_.clear();
    else
    {
        currentSong_ = songs_.at(order_.front());
        order_.erase(order_.begin());
    }
    return currentSong_;
}
