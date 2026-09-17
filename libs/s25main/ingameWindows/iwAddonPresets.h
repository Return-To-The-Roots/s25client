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
    static constexpr unsigned contentX = 20;
    static constexpr unsigned contentWidth = 400;
    static constexpr unsigned tableY = 30;
    static constexpr unsigned tableHeight = 200;
    static constexpr unsigned rowGap = 8;
    static constexpr unsigned rowHeight = 22;
    static constexpr unsigned bottomMargin = 24;
    /// Subclasses add their own controls here; the bottom row is reserved for the buttons
    static constexpr unsigned contentStartY = tableY + tableHeight + rowGap + rowHeight;
    static constexpr unsigned halfWidth = (contentWidth - rowGap) / 2;
    static constexpr unsigned rightColumnX = contentX + halfWidth + rowGap;

    iwAddonPresetsBase(const std::string& title, const std::string& actionLabel, unsigned height);

    virtual void RefreshTable();
    /// Empty if no preset is selected
    boost::filesystem::path GetSelectedFilePath() const;

    void Msg_ButtonClick(unsigned ctrl_id) override;
    void Msg_TableSelectItem(unsigned ctrl_id, const std::optional<unsigned>& selection) override;
    void Msg_TableChooseItem(unsigned ctrl_id, unsigned selection) override;
    void Msg_MsgBoxResult(unsigned msgbox_id, MsgboxResult mbr) override;

private:
    virtual void DoAction() = 0;
    void ConfirmDelete();
};

class iwSaveAddonPreset : public iwAddonPresetsBase
{
public:
    explicit iwSaveAddonPreset(std::map<unsigned, unsigned> states);

private:
    const std::map<unsigned, unsigned> states_;
    bool deselectingFromEdit_ = false;
    void RefreshTable() override;
    void SaveToPath(const boost::filesystem::path& filePath);
    void DoAction() override;
    void Msg_EditEnter(unsigned ctrl_id) override;
    void Msg_EditChange(unsigned ctrl_id) override;
    void Msg_TableSelectItem(unsigned ctrl_id, const std::optional<unsigned>& selection) override;
    void Msg_MsgBoxResult(unsigned msgbox_id, MsgboxResult mbr) override;
};

class iwLoadAddonPreset : public iwAddonPresetsBase
{
public:
    explicit iwLoadAddonPreset(std::function<void(const std::map<unsigned, unsigned>&)> onLoad);

private:
    std::function<void(const std::map<unsigned, unsigned>&)> onLoad_;
    void DoAction() override;
    void Msg_TableSelectItem(unsigned ctrl_id, const std::optional<unsigned>& selection) override;
};
