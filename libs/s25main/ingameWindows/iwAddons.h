// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "IngameWindow.h"
#include "addons/const_addons.h"
#include <memory>
#include <vector>

class AddonGui;
class GlobalGameSettings;
struct MouseCoords;

enum class AddonChangeAllowed
{
    All,
    WhitelistOnly,
    None,
    AllAndSaveToConfig
};

class iwAddons : public IngameWindow
{
public:
    iwAddons(GlobalGameSettings& ggs, Window* parent = nullptr,
             AddonChangeAllowed policy = AddonChangeAllowed::AllAndSaveToConfig,
             std::vector<AddonId> whitelistedAddons = {});
    ~iwAddons() override;

protected:
    void Msg_ButtonClick(unsigned ctrl_id) override;
    void Msg_OptionGroupChange(unsigned ctrl_id, unsigned selection) override;
    void Msg_ScrollChange(unsigned ctrl_id, unsigned short position) override;
    bool Msg_WheelUp(const MouseCoords& mc) override;
    bool Msg_WheelDown(const MouseCoords& mc) override;

private:
    /// settings we edit in this window
    GlobalGameSettings& ggs;
    AddonChangeAllowed policy_;
    std::vector<AddonId> whitelistedAddons_;
    std::vector<std::unique_ptr<AddonGui>> addonGuis_;

    /// Aktualisiert die Addons, die angezeigt werden sollen
    void UpdateView(AddonGroup selection);
    bool isReadOnly(AddonId) const;
};
