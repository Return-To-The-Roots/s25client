// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#include "rttrDefines.h" // IWYU pragma: keep
#include "MusicPlayer.h"
#include "Loader.h"
#include "drivers/AudioDriverWrapper.h"
#include "ingameWindows/iwMusicPlayer.h"
#include "ogl/MusicItem.h"
#include "libsiedler2/Archiv.h"
#include "libsiedler2/ArchivItem.h"
#include "libsiedler2/ErrorCodes.h"
#include "libsiedler2/prototypen.h"
#include "libutil/Log.h"
#include "libutil/StringConversion.h"
#include <boost/algorithm/string/trim.hpp>
#include <boost/nowide/fstream.hpp>
#include <algorithm>
#include <sstream>

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
        std::random_shuffle(order.begin(), order.end());
}

const std::string Playlist::getCurrentSong() const
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

    bnw::ofstream out(filename.c_str());
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

static constexpr bool isNewline(char c)
{
    return c == '\r' || c == '\n';
}

/**
 *  Playlist laden
 */
bool Playlist::Load(const std::string& filename)
{
    songs.clear();
    if(filename.empty())
        return false;

    LOG.write(_("Loading \"%s\"\n")) % filename;

    bfs::path filepath(filename);
    if(filepath.extension() != ".pll")
        filepath.replace_extension("pll");
    bnw::ifstream in(filepath);

    if(in.fail())
        return false;

    std::string line, random_str;
    std::stringstream sline;

    if(!std::getline(in, line))
        return false;
    sline.clear();
    sline << line;
    if(!(sline >> repeats >> random_str))
        return false;

    random = (random_str == "random_playback" || random_str == "random");

    while(std::getline(in, line))
    {
        boost::algorithm::trim_if(line, &isNewline);
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
const std::string Playlist::getNextSong()
{
    const std::string tmp = getCurrentSong();
    current = tmp.empty() ? -1 : static_cast<int>(order.front());
    if(!order.empty())
        order.erase(order.begin());
    return tmp;
}

MusicPlayer::MusicPlayer() : playing(false) {}

/**
 *  Startet Abspielvorgang
 */
void MusicPlayer::Play()
{
    playing = true;
    PlayNext();
}

/**
 *  Stoppt Abspielvorgang
 */
void MusicPlayer::Stop()
{
    AUDIODRIVER.StopMusic();
    playing = false;
}

/**
 * Spielt nächstes Stück ab
 */
void MusicPlayer::PlayNext()
{
    std::string song = list.getNextSong();

    // Am Ende der Liste angekommen?
    if(song.empty())
    {
        Stop();
        return;
    }

    // Evtl ein Siedlerstück ("sNN")?
    if(song.length() == 3 && song[0] == 's')
    {
        unsigned nr = s25util::fromStringClassicDef(song.substr(1), 999u);
        if(nr <= 14)
        {
            // Siedlerstück abspielen (falls es geladen wurde)
            MusicItem* curSong = dynamic_cast<MusicItem*>(LOADER.sng_lst[nr - 1]);
            if(curSong)
                curSong->Play(1);
        }
        return;
    }

    // anderes benutzerdefiniertes Stück abspielen
    // in "sng" speichern, daher evtl. altes Stück erstmal löschen
    sng.clear();

    LOG.write(_("Loading \"%s\": ")) % song;

    // Neues Stück laden
    if(int ec = libsiedler2::loader::LoadSND(song, sng))
    {
        LOG.write(_("Error: %1%\n")) % libsiedler2::getErrorString(ec);
        Stop();
        return;
    }
    LOG.write(_("OK\n"));

    // Und abspielen
    MusicItem* curSong = dynamic_cast<MusicItem*>(sng[0]);
    if(curSong)
        curSong->Play(1);
}
