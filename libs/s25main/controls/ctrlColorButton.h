// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "controls/ctrlButton.h"
#include "ctrlBaseColor.h"

/// Button mit Farbe
class ctrlColorButton : public ctrlButton, public ctrlBaseColor
{
public:
    ctrlColorButton(Window* parent, unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
                    unsigned fillColor, const std::string& tooltip);

protected:
    void DrawContent() const override;
};
