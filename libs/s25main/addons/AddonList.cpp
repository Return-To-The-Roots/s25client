// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "AddonList.h"
#include "Loader.h"
#include "Window.h"
#include "commonDefines.h"
#include "controls/ctrlComboBox.h"
#include <stdexcept>

AddonList::AddonList(const AddonId id, AddonGroup groups, const std::string& name, const std::string& description,
                     std::vector<std::string> options, unsigned defaultStatus /*=0*/)
    : Addon(id, groups, name, description, defaultStatus), options(std::move(options))
{
    if(defaultStatus >= this->options.size())
        throw std::logic_error("Invalid default option");
}

std::unique_ptr<AddonGui> AddonList::createGui(Window& window, bool readonly) const
{
    return std::make_unique<Gui>(*this, window, readonly);
}

unsigned AddonList::getNumOptions() const
{
    return options.size();
}

const std::string& AddonList::getOptionName(unsigned status) const
{
    return options.at(status);
}

AddonList::Gui::Gui(const AddonList& addon, Window& window, bool readonly)
    : AddonGui(addon, window, readonly),
      cb_(assertNonNull(
        window.AddComboBox(2, DrawPoint(430, 0), Extent(220, 20), TextureColor::Grey, NormalFont, 100, readonly)))
{
    for(const auto& option : addon.options)
        cb_.AddItem(option);
    if(readonly)
        window.AddImage(3, cb_.GetPos() - DrawPoint(1, 0), LOADER.GetImageN("io_new", 14), _("Locked"));
}

void AddonList::Gui::setStatus(unsigned status)
{
    cb_.SetSelection(status);
}

unsigned AddonList::Gui::getStatus() const
{
    return cb_.GetSelection().value();
}
