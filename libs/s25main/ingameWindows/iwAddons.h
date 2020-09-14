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

#pragma once

#include "IngameWindow.h"
#include "addons/const_addons.h"
#include <memory>
#include <vector>

class AddonGui;
class GlobalGameSettings;
class MouseCoords;

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
    iwAddons(GlobalGameSettings& ggs, Window* parent = nullptr, AddonChangeAllowed policy = AddonChangeAllowed::AllAndSaveToConfig,
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
