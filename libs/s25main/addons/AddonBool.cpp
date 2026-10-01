// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "AddonBool.h"
#include "Loader.h"
#include "Window.h"
#include "commonDefines.h"
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

AddonBool::Gui::Gui(const Addon& addon, Window& window, bool readonly)
    : AddonGui(addon, window, readonly),
      cb_(assertNonNull(
        window.AddCheckBox(2, DrawPoint(430, 0), Extent(220, 20), TextureColor::Grey, _("Use"), NormalFont, readonly)))
{
    if(readonly)
        window.AddImage(3, cb_.GetPos() - DrawPoint(1, 0), LOADER.GetImageN("io_new", 14), _("Locked"));
}

void AddonBool::Gui::setStatus(unsigned status)
{
    cb_.setChecked(status != 0);
}

unsigned AddonBool::Gui::getStatus() const
{
    return cb_.isChecked() ? 1 : 0;
}
