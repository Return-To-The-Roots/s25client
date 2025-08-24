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
    void SetTooltip(const std::string& tooltip);
    const std::string& GetTooltip() const { return tooltip_; }
    /// Swap the tooltips of those controls
    void SwapTooltip(ctrlBaseTooltip& other);

    void ShowTooltip() const;
    /// Show a temporary tooltip
    void ShowTooltip(const std::string& tooltip) const;
    void HideTooltip() const;

protected:
    std::string tooltip_;
};
