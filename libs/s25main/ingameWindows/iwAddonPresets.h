// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "IngameWindow.h"
#include <boost/filesystem/path.hpp>
#include <functional>
#include <map>
#include <optional>
#include <string>

/// Base class for the save/load addon preset windows
class iwAddonPresetsBase : public IngameWindow
{
public:
    enum
    {
        ID_tblPresets,
        ID_edtName,
        ID_btAction,
        ID_btDelete,
        ID_txtFolder,
        ID_mbDelete,
        ID_mbOverwrite,
    };

protected:
    /// additionalHeight is the room the subclass needs for its own controls above the buttons
    iwAddonPresetsBase(const std::string& title, const std::string& actionLabel, unsigned additionalHeight);

    void refreshTable();
    /// Empty if no preset is selected
    boost::filesystem::path getSelectedFilePath() const;

    void msgButtonClick(unsigned ctrl_id) override;
    void msgTableSelectItem(unsigned ctrl_id, const std::optional<unsigned>& selection) override;
    void msgTableChooseItem(unsigned ctrl_id, unsigned selection) override;
    void msgMsgBoxResult(unsigned msgbox_id, MsgboxResult mbr) override;

private:
    bool isPresetSelected() const;
    void updateButtons();
    virtual bool canDoAction() const;
    virtual void doAction() = 0;
    void confirmDelete();
};

class iwSaveAddonPreset : public iwAddonPresetsBase
{
public:
    explicit iwSaveAddonPreset(std::map<unsigned, unsigned> states);

private:
    const std::map<unsigned, unsigned> states_;
    void saveToPath(const boost::filesystem::path& filePath);
    bool canDoAction() const override;
    void doAction() override;
    void msgEditEnter(unsigned ctrl_id) override;
    void msgEditChange(unsigned ctrl_id) override;
    void msgTableSelectItem(unsigned ctrl_id, const std::optional<unsigned>& selection) override;
    void msgMsgBoxResult(unsigned msgbox_id, MsgboxResult mbr) override;
};

class iwLoadAddonPreset : public iwAddonPresetsBase
{
public:
    explicit iwLoadAddonPreset(std::function<void(const std::map<unsigned, unsigned>&)> onLoad);

private:
    std::function<void(const std::map<unsigned, unsigned>&)> onLoad_;
    void doAction() override;
};
