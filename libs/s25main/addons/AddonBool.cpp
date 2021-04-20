// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
