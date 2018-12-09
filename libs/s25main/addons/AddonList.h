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

#ifndef AddonList_h__
#define AddonList_h__

#include "Addon.h"
#include <vector>

/**
 *  Addon baseclass for option-list addons
 *  first option added will be the default one
 */
class AddonList : public Addon
{
public:
    AddonList(const AddonId id, const unsigned groups, const std::string& name, const std::string& description,
              const unsigned default_status)
        : Addon(id, groups, name, description, default_status)
    {}

    void hideGui(Window* window, unsigned id) const override;
    void createGui(Window* window, unsigned id, unsigned short& y, bool readonly, unsigned status) const override;
    void setGuiStatus(Window* window, unsigned id, unsigned status) const override;
    unsigned getGuiStatus(Window* window, unsigned id, bool& failed) const override;

    unsigned getNumOptions() const override;

protected:
    void removeOptions();
    void addOption(const std::string& name);

private:
    std::vector<std::string> options;
};

#endif // AddonList_h__
