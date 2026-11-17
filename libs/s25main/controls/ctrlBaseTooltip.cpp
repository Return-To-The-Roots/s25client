// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ctrlBaseTooltip.h"
#include "WindowManager.h"

ctrlBaseTooltip::~ctrlBaseTooltip()
{
    hideTooltip();
}

void ctrlBaseTooltip::setTooltip(const std::string& tooltip)
{
    tooltip_ = tooltip;
    WINDOWMANAGER.setToolTip(this, tooltip_, true);
}

void ctrlBaseTooltip::swapTooltip(ctrlBaseTooltip& other)
{
    std::swap(tooltip_, other.tooltip_);
}

void ctrlBaseTooltip::showTooltip() const
{
    showTooltip(tooltip_);
}

void ctrlBaseTooltip::showTooltip(const std::string& tooltip) const
{
    WINDOWMANAGER.setToolTip(this, tooltip);
}

void ctrlBaseTooltip::hideTooltip() const
{
    WINDOWMANAGER.setToolTip(this, "");
}
