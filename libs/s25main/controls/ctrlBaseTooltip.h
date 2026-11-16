// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>
#include <utility>

/// Base class for controls with a tooltip
class ctrlBaseTooltip
{
public:
    ctrlBaseTooltip(std::string tooltip = "") : tooltip_(std::move(tooltip)) {}
    virtual ~ctrlBaseTooltip();

    /// Set the text to be shown, updates the tooltip if currently shown
    void setTooltip(const std::string& tooltip);
    const std::string& getTooltip() const { return tooltip_; }
    /// Swap the tooltips of those controls
    void swapTooltip(ctrlBaseTooltip& other);

    void showTooltip() const;
    /// Show a temporary tooltip
    void showTooltip(const std::string& tooltip) const;
    void hideTooltip() const;

protected:
    std::string tooltip_;
};
