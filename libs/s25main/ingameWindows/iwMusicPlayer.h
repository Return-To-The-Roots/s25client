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

#pragma once

#include "IngameWindow.h"
#include <boost/filesystem/path.hpp>

class Playlist;

/// Fenster zum Einstellen des Musik-Players
class iwMusicPlayer final : public IngameWindow
{
    /// Kleines Fenster zur Eingabe von Text
    class InputWindow final : public IngameWindow
    {
        const unsigned win_id;
        iwMusicPlayer& playerWnd_;

    public:
        InputWindow(iwMusicPlayer& playerWnd, unsigned win_id, const std::string& title);

        void Msg_ButtonClick(unsigned ctrl_id) override;
        void Msg_EditEnter(unsigned ctrl_id) override;
    };

    /// Merken, ob Veränderungen an den Musikeinstellungen durchgeführt wurden und ob deswegen
    /// beim Schließen des Fensters das ganze neu gestartet werden muss
    bool changed;

public:
    iwMusicPlayer();
    ~iwMusicPlayer() override;

private:
    /// Get the full path to a playlist by its name
    static boost::filesystem::path GetFullPlaylistPath(const std::string& name);

    unsigned GetRepeats() const;
    void SetRepeats(unsigned repeats);
    bool GetRandomPlayback() const;
    void SetRandomPlayback(bool random_playback);

    /// Updatet die Playlist- Combo, selektiert entsprechenden Eintrag, falls vorhanden
    void UpdatePlaylistCombo(const std::string& highlight_entry);

    bool SaveCurrentPlaylist();
    void UpdateFromPlaylist(const Playlist&);
    Playlist MakePlaylist();

    void Msg_ListChooseItem(unsigned ctrl_id, unsigned selection) override;
    void Msg_ComboSelectItem(unsigned ctrl_id, unsigned selection) override;
    void Msg_ButtonClick(unsigned ctrl_id) override;

    void Msg_Input(unsigned win_id, const std::string& msg);
};
