// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "controls/ctrlBaseColor.h"
#include "controls/ctrlDeepening.h"

/// Colored Deepening
class ctrlColorDeepening : public ctrlDeepening, public ctrlBaseColor
{
public:
    ctrlColorDeepening(Window* parent, unsigned id, DrawPoint pos, const Extent& size, TextureColor tc,
                       unsigned fillColor);

protected:
    void DrawContent() const override;
};
