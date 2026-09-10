// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Addon.h"

class ctrlCheck;

/**
 *  Addon baseclass for boolean addons
 */
class AddonBool : public Addon
{
    class Gui : public AddonGui
    {
        ctrlCheck& cb_;

    public:
        Gui(const Addon& addon, Window& window, bool readonly);
        void setStatus(unsigned status) override;
        unsigned getStatus() const override;
    };

public:
    AddonBool(AddonId id, AddonGroup groups, const std::string& name, const std::string& description,
              unsigned defaultStatus = 0);

    unsigned getNumOptions() const override;

    std::unique_ptr<AddonGui> createGui(Window& window, bool readonly) const override;
};
