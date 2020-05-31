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

#include "Playlist.h"
#include "ingameWindows/iwMusicPlayer.h"
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

/*#include "Loader.h"
#include "drivers/AudioDriverWrapper.h"
#include "ogl/MusicItem.h"
#include "libsiedler2/Archiv.h"
#include "libsiedler2/ArchivItem.h"
#include "libsiedler2/ErrorCodes.h"
#include "libsiedler2/prototypen.h"
#include "s25util/StringConversion.h"
*/

Playlist::Playlist() : current(-1), repeats(1), random(false) {}

/**
 *  startet das Abspielen der Playlist.
 */
void Playlist::Prepare()
{
    // Add one entry per song repeated if required
    order.resize(songs.size() * repeats);

    for(unsigned i = 0; i < songs.size() * repeats; ++i)
        order[i] = i % songs.size();

    // Shuffle if requested
    if(random)
        std::shuffle(order.begin(), order.end(), std::mt19937(std::random_device()()));
}

std::string Playlist::getCurrentSong() const
{
    if(order.empty() || order.front() >= songs.size())
        return "";
    return songs[order.front()];
}

/**
 *  Playlist in Datei speichern
 */
bool Playlist::SaveAs(const std::string& filename, const bool overwrite)
{
    if(!overwrite)
    {
        bnw::ifstream in(filename.c_str());
        if(in.good())
        {
            // Datei existiert und wir sollen sie nicht überschreiben
            in.close();
            return false;
        }
    }

    s25util::ClassicImbuedStream<bnw::ofstream> out(filename.c_str());
    if(!out.good())
        return false;

    out << repeats << " ";
    out << (random ? "random" : "ordered") << std::endl;

    // songs reinschreiben
    for(const auto& song : songs)
        out << song << "\n";

    out.close();

    return true;
}

/**
 *  Playlist laden
 */
bool Playlist::Load(Log& logger, const std::string& filename)
{
    songs.clear();
    if(filename.empty())
        return false;

    logger.write(_("Loading \"%s\"\n")) % filename;

    boost::filesystem::path filepath(filename);
    if(filepath.extension() != ".pll")
        filepath.replace_extension("pll");
    bnw::ifstream in(filepath);

    if(in.fail())
        return false;

    std::string line, random_str;
    s25util::ClassicImbuedStream<std::stringstream> sline;

    if(!std::getline(in, line))
        return false;
    sline.clear();
    sline << line;
    if(!(sline >> repeats >> random_str))
        return false;

    random = (random_str == "random_playback" || random_str == "random");

    while(std::getline(in, line))
    {
        boost::algorithm::trim_if(line, [](char c) { return c == '\r' || c == '\n'; });
        if(line.empty())
            break;
        songs.push_back(line);
    }

    if(in.bad())
        return false;

    // geladen, also zum Abspielen vorbereiten
    Prepare();
    return true;
}

/**
 *  Füllt das iwMusicPlayer-Fenster mit den entsprechenden Werten
 */
void Playlist::FillMusicPlayer(iwMusicPlayer* window) const
{
    window->SetSegments(songs);
    window->SetRepeats(repeats);
    window->SetRandomPlayback(random);

    if(current >= 0)
        window->SetCurrentSong(current);
}

/**
 *  Liest die Werte aus dem iwMusicPlayer-Fenster
 */
void Playlist::ReadMusicPlayer(const iwMusicPlayer* const window)
{
    repeats = window->GetRepeats();
    random = window->GetRandomPlayback();
    songs = window->GetSegments();

    // zum Abspielen vorbereiten
    Prepare();
}

// Wählt den Start-Song aus
void Playlist::SetStartSong(const unsigned id)
{
    for(unsigned int& i : order)
    {
        if(i == id)
        {
            std::swap(order[0], i);
            return;
        }
    }
}

/// schaltet einen Song weiter und liefert den Dateinamen des aktuellen Songs
std::string Playlist::getNextSong()
{
    const std::string tmp = getCurrentSong();
    current = tmp.empty() ? -1 : static_cast<int>(order.front());
    if(!order.empty())
        order.erase(order.begin());
    return tmp;
}
