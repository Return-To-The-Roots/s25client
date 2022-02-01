// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
    void Close() override;

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
