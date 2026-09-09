// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Addon.h"
#include <vector>

class ctrlComboBox;

/**
 *  Addon baseclass for option-list addons
 */
class AddonList : public Addon
{
    class Gui : public AddonGui
    {
        ctrlComboBox& cb_;

    public:
        Gui(const AddonList& addon, Window& window, bool readonly);
        void setStatus(unsigned status) override;
        unsigned getStatus() const override;
    };

public:
    AddonList(AddonId id, AddonGroup groups, const std::string& name, const std::string& description,
              std::vector<std::string> options, unsigned defaultStatus = 0);

    unsigned getNumOptions() const override;
    const std::string& getOptionName(unsigned status) const;

    std::unique_ptr<AddonGui> createGui(Window& window, bool readonly) const override;

private:
    std::vector<std::string> options;
};
