// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ctrlBaseTooltip.h"
#include "WindowManager.h"

ctrlBaseTooltip::~ctrlBaseTooltip()
{
    HideTooltip();
}

void ctrlBaseTooltip::SwapTooltip(ctrlBaseTooltip& other)
{
    std::swap(tooltip_, other.tooltip_);
}

void ctrlBaseTooltip::ShowTooltip() const
{
    ShowTooltip(tooltip_);
}

void ctrlBaseTooltip::ShowTooltip(const std::string& tooltip) const
{
    WINDOWMANAGER.SetToolTip(this, tooltip);
}

void ctrlBaseTooltip::HideTooltip() const
{
    WINDOWMANAGER.SetToolTip(this, "");
}
