// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "AddonList.h"
#include "Loader.h"
#include "Window.h"
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

AddonList::Gui::Gui(const AddonList& addon, Window& window, bool readonly) : AddonGui(addon, window, readonly)
{
    DrawPoint cbPos(430, 0);

    auto* cb = window.AddComboBox(2, cbPos, Extent(220, 20), TextureColor::Grey, NormalFont, 100, readonly);
    for(const auto& option : addon.options)
        cb->AddString(option);
}

void AddonList::Gui::setStatus(Window& window, unsigned status)
{
    auto* cb = window.GetCtrl<ctrlComboBox>(2);
    RTTR_Assert(cb);
    cb->SetSelection(status);
}

unsigned AddonList::Gui::getStatus(const Window& window)
{
    const auto* cb = window.GetCtrl<ctrlComboBox>(2);
    RTTR_Assert(cb);
    return cb->GetSelection().get();
}
