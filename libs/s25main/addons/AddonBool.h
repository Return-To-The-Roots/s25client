// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

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
