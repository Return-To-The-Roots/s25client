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
    iwAddonPresetsBase(const std::string& title, unsigned height);

    void RefreshTable();

    void Msg_ButtonClick(unsigned ctrl_id) override;
    void Msg_TableChooseItem(unsigned ctrl_id, unsigned selection) override;

private:
    virtual void DoAction() = 0;
};

class iwSaveAddonPreset : public iwAddonPresetsBase
{
public:
    explicit iwSaveAddonPreset(std::map<unsigned, unsigned> states);

private:
    const std::map<unsigned, unsigned> states_;
    void SaveToPath(const boost::filesystem::path& filePath);
    void DoAction() override;
    void Msg_EditEnter(unsigned ctrl_id) override;
    void Msg_TableSelectItem(unsigned ctrl_id, const std::optional<unsigned>& selection) override;
    void Msg_MsgBoxResult(unsigned msgbox_id, MsgboxResult mbr) override;
};

class iwLoadAddonPreset : public iwAddonPresetsBase
{
public:
    explicit iwLoadAddonPreset(std::function<void(const std::map<unsigned, unsigned>&)> onLoad);

private:
    std::function<void(const std::map<unsigned, unsigned>&)> onLoad_;
    /// Empty if no preset is selected
    boost::filesystem::path GetSelectedFilePath() const;
    void ConfirmDelete();
    void DoAction() override;
    void Msg_ButtonClick(unsigned ctrl_id) override;
    void Msg_TableSelectItem(unsigned ctrl_id, const std::optional<unsigned>& selection) override;
    void Msg_MsgBoxResult(unsigned msgbox_id, MsgboxResult mbr) override;
};
