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
#ifndef MUSICPLAYER_H_
#define MUSICPLAYER_H_

#include "IngameWindow.h"

/// Fenster zum Einstellen des Musik-Players
class iwMusicPlayer : public IngameWindow
{
    /// Kleines Fenster zur Eingabe von Text
    class InputWindow : public IngameWindow
    {
        const unsigned win_id;
        iwMusicPlayer& playerWnd_;

    public:
        InputWindow(iwMusicPlayer& playerWnd, const unsigned win_id, const std::string& title);

        void Msg_ButtonClick(const unsigned ctrl_id) override;
        void Msg_EditEnter(const unsigned ctrl_id) override;
    };

    /// Merken, ob Veränderungen an den Musikeinstellungen durchgeführt wurden und ob deswegen
    /// beim Schließen des Fensters das ganze neu gestartet werden muss
    bool changed;

public:
    iwMusicPlayer();
    ~iwMusicPlayer() override;

    /// Setzt Werte
    void SetSegments(const std::vector<std::string>& segments);
    void SetRepeats(unsigned repeats);
    void SetRandomPlayback(const bool random_playback);
    void SetCurrentSong(const unsigned selection);

    /// Gibt Werte zurück
    std::vector<std::string> GetSegments() const;
    unsigned GetRepeats() const;
    bool GetRandomPlayback() const;

    /// Updatet die Playlist- Combo, selektiert entsprechenden Eintrag, falls vorhanden
    void UpdatePlaylistCombo(const std::string& highlight_entry);

    /// Get the full path to a playlist by its name
    static std::string GetFullPlaylistPath(const std::string& name);

private:
    void Msg_ListChooseItem(const unsigned ctrl_id, const unsigned selection) override;
    void Msg_ComboSelectItem(const unsigned ctrl_id, const int selection) override;
    void Msg_ButtonClick(const unsigned ctrl_id) override;

    void Msg_Input(const unsigned win_id, const std::string& msg);
};

#endif
