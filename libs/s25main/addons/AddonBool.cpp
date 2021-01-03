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

#include "AddonBool.h"
#include "Loader.h"
#include "Window.h"
#include "controls/ctrlCheck.h"

AddonBool::AddonBool(const AddonId id, AddonGroup groups, const std::string& name, const std::string& description)
    : Addon(id, groups, name, description, 0)
{}

std::unique_ptr<AddonGui> AddonBool::createGui(Window& window, bool readonly) const
{
    return std::make_unique<Gui>(*this, window, readonly);
}

unsigned AddonBool::getNumOptions() const
{
    return 2;
}

AddonBool::Gui::Gui(const Addon& addon, Window& window, bool readonly) : AddonGui(addon, window, readonly)
{
    DrawPoint cbPos(430, 0);
    window.AddCheckBox(2, cbPos, Extent(220, 20), TextureColor::Grey, _("Use"), NormalFont, readonly);
}

void AddonBool::Gui::setStatus(Window& window, unsigned status)
{
    auto* cb = window.GetCtrl<ctrlCheck>(2);
    RTTR_Assert(cb);
    cb->SetCheck(status != 0);
}

unsigned AddonBool::Gui::getStatus(const Window& window)
{
    const auto* cb = window.GetCtrl<ctrlCheck>(2);
    RTTR_Assert(cb);
    return cb->GetCheck() ? 1 : 0;
}
