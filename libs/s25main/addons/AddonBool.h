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

#ifndef AddonBool_h__
#define AddonBool_h__

#include "Addon.h"

/**
 *  Addon baseclass for boolean addons
 */
class AddonBool : public Addon
{
    class Gui : public AddonGui
    {
    public:
        Gui(const Addon& addon, Window& window, bool readonly);
        void setStatus(Window& window, unsigned status) override;
        unsigned getStatus(const Window& window) override;
    };

public:
    AddonBool(AddonId id, AddonGroup groups, const std::string& name, const std::string& description);

    unsigned getNumOptions() const override;

    std::unique_ptr<AddonGui> createGui(Window& window, bool readonly) const override;
};

#endif // AddonBool_h__
