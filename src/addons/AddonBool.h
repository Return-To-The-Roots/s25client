// Copyright (c) 2005 - 2016 Settlers Freaks (sf-team at siedler25.org)
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

#ifndef AddonBool_h__
#define AddonBool_h__

#include "Addon.h"

/**
 *  Addon baseclass for boolean addons
 */
class AddonBool : public Addon
{
    public:
        AddonBool(const AddonId id, const unsigned int groups, const std::string& name, const std::string& description, const unsigned int default_status)
            : Addon(id, groups, name, description, default_status) { }

        void hideGui(Window* window, unsigned int id) const override;
        void createGui(Window* window, unsigned int id, unsigned short& y, bool readonly, unsigned int status) const override;
        void setGuiStatus(Window* window, unsigned int id, unsigned int status) const override;
        unsigned int getGuiStatus(Window* window, unsigned int id, bool& failed) const override;

        unsigned getNumOptions() const override;
};

#endif // AddonBool_h__
