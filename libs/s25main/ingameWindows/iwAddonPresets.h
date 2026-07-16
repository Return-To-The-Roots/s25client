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
    explicit iwAddonPresetsBase(const std::string& title, const std::string& actionLabel);

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
    void RefreshTable();
    /// Resolves the preset file the name edit points at, or empty if the name is
    /// empty/invalid or no preset with that name exists.
    boost::filesystem::path GetTargetFilePath() const;
    /// Like GetTargetFilePath(), but if a name was entered that doesn't resolve to an existing
    /// preset (e.g. the user edited the name), informs the user before returning empty.
    /// An empty name stays a silent no-op.
    boost::filesystem::path GetTargetFileOrNotify();

    void Msg_EditEnter(unsigned ctrl_id) override;
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
    void SaveToPath(const boost::filesystem::path& filePath);
    void DoAction() override;
    void Msg_MsgBoxResult(unsigned msgbox_id, MsgboxResult mbr) override;
};

class iwLoadAddonPreset : public iwAddonPresetsBase
{
public:
    explicit iwLoadAddonPreset(std::function<void(const std::map<unsigned, unsigned>&)> onLoad);

private:
    std::function<void(const std::map<unsigned, unsigned>&)> onLoad_;
    void DoAction() override;
};
