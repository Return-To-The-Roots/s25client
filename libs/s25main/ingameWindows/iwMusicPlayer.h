// Copyright (C) 2005 - 2025 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "IngameWindow.h"
#include <boost/filesystem/path.hpp>

class Playlist;

/// Window for changing music player options and playlists
class iwMusicPlayer final : public IngameWindow
{
    /// Small window for text input
    class InputWindow final : public IngameWindow
    {
        const unsigned win_id;
        iwMusicPlayer& playerWnd_;

    public:
        InputWindow(iwMusicPlayer& playerWnd, unsigned win_id, const std::string& title);

        void Msg_ButtonClick(unsigned ctrl_id) override;
        void Msg_EditEnter(unsigned ctrl_id) override;
    };

    /// Set to true if anything changed to (re)start music on close
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

    /// Fill combobox with all playlists and selects given entry if present
    void UpdatePlaylistCombo(const std::string& highlight_entry);

    bool SaveCurrentPlaylist();
    void UpdateFromPlaylist(const Playlist&);
    // Enable/Disable buttons for changing playlist contents
    void UpdateSaveChangeButtonsState();
    Playlist MakePlaylist();

    void Msg_ListChooseItem(unsigned ctrl_id, unsigned selection) override;
    void Msg_ComboSelectItem(unsigned ctrl_id, unsigned selection) override;
    void Msg_ButtonClick(unsigned ctrl_id) override;

    void Msg_Input(unsigned win_id, const std::string& msg);
};
