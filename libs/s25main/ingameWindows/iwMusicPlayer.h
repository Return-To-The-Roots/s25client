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

        void msgButtonClick(unsigned ctrl_id) override;
        void msgEditEnter(unsigned ctrl_id) override;
    };

    /// Set to true if anything changed to (re)start music on close
    bool changed;

public:
    iwMusicPlayer();
    void Close() override;

private:
    /// Get the full path to a playlist by its name
    static boost::filesystem::path getFullPlaylistPath(const std::string& name);

    unsigned getRepeats() const;
    void setRepeats(unsigned repeats);
    bool getRandomPlayback() const;
    void setRandomPlayback(bool random_playback);

    /// Fill combobox with all playlists and selects given entry if present
    void UpdatePlaylistCombo(const std::string& highlight_entry);

    bool SaveCurrentPlaylist();
    void UpdateFromPlaylist(const Playlist&);
    // Enable/Disable buttons for changing playlist contents
    void UpdateSaveChangeButtonsState();
    Playlist MakePlaylist();

    void msgListChooseItem(unsigned ctrl_id, unsigned selection) override;
    void msgComboSelectItem(unsigned ctrl_id, unsigned selection) override;
    void msgButtonClick(unsigned ctrl_id) override;

    void msgInput(unsigned win_id, const std::string& msg);
};
