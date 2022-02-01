// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Playlist.h"
#include "helpers/containerUtils.h"
#include "mygettext/mygettext.h"
#include "s25util/Log.h"
#include "s25util/StringConversion.h"
#include <boost/algorithm/string/trim.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/nowide/fstream.hpp>
#include <algorithm>
#include <random>
#include <sstream>
#include <stdexcept>

namespace bnw = boost::nowide;

Playlist::Playlist(std::vector<std::string> songs, unsigned numRepeats, bool random)
    : songs_(std::move(songs)), numRepeats_(numRepeats), random_(random)
{
    if(helpers::contains(songs_, std::string{}))
        throw std::invalid_argument("Empty song passed");
}

void Playlist::Prepare()
{
    currentSong_.clear();
    order_.clear();

    // Add one entry per song repeated if required
    order_.reserve(songs_.size() * (numRepeats_ + 1u));
    for(unsigned i = 0; i < songs_.size(); ++i)
        order_.insert(order_.end(), numRepeats_ + 1u, i);

    // Shuffle if requested
    if(random_)
        std::shuffle(order_.begin(), order_.end(), std::mt19937(std::random_device()()));
}

bool Playlist::SaveAs(const boost::filesystem::path& filepath) const
{
    s25util::ClassicImbuedStream<bnw::ofstream> out(filepath);
    if(!out)
        return false;

    out << numRepeats_ << " ";
    out << (random_ ? "random" : "ordered") << std::endl;

    for(const auto& song : songs_)
        out << song << "\n";

    return static_cast<bool>(out);
}

bool Playlist::Load(Log& logger, const boost::filesystem::path& filepath)
{
    songs_.clear();
    order_.clear();
    if(filepath.empty())
        return false;

    logger.write(_("Loading %1%\n")) % filepath;

    bnw::ifstream in(filepath);
    if(!in)
        return false;

    std::string line, random_str;
    if(!std::getline(in, line))
        return false;
    s25util::ClassicImbuedStream<std::istringstream> sline(line);
    if(!(sline >> numRepeats_ >> random_str))
        return false;

    random_ = (random_str == "random_playback" || random_str == "random");

    while(std::getline(in, line))
    {
        boost::algorithm::trim_if(line, [](char c) { return c == '\r' || c == '\n'; });
        if(!line.empty())
            songs_.push_back(line);
    }

    return in.eof();
}

void Playlist::SetStartSong(const unsigned id)
{
    const auto itEl = helpers::find(order_, id);
    if(itEl != order_.end())
        std::swap(order_.front(), *itEl);
}

const std::string& Playlist::getNextSong()
{
    // Playlist not started, yet or full played?
    if(order_.empty())
        Prepare();

    // Still no songs? (I.e. empty playlist)
    if(order_.empty())
        currentSong_.clear();
    else
    {
        currentSong_ = songs_[order_.front()];
        order_.erase(order_.begin());
    }
    return currentSong_;
}
