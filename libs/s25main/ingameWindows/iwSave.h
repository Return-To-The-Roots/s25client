// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "IngameWindow.h"
#include "network/CreateServerInfo.h"
#include <boost/filesystem/path.hpp>

/// Base class for the window for saving and loading games
class iwSaveLoad : public IngameWindow
{
public:
    iwSaveLoad(const std::string& window_title, ITexture* btImg, unsigned addHeight = 0);

protected:
    /// Re-fill the table with existing files
    void RefreshTable();

private:
    /// Save or load the currently selected file
    virtual void SaveLoad() = 0;

    void Msg_EditEnter(unsigned ctrl_id) override;
    void Msg_ButtonClick(unsigned ctrl_id) override;
    void Msg_TableSelectItem(unsigned ctrl_id, const boost::optional<unsigned>& selection) override;
};

class iwSave : public iwSaveLoad
{
public:
    iwSave();

private:
    // Save game
    void SaveLoad() override;

    void Msg_ComboSelectItem(unsigned ctrl_id, unsigned selection) override;
};

class iwLoad : public iwSaveLoad
{
    const CreateServerInfo csi;

public:
    iwLoad(CreateServerInfo csi);

private:
    /// Handle double click on the table
    void Msg_TableChooseItem(unsigned ctrl_id, unsigned selection) override;

    // Load game
    void SaveLoad() override;
};
