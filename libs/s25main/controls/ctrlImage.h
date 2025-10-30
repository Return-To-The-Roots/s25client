// Copyright (C) 2005 - 2025 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Window.h"
#include "controls/ctrlBaseImage.h"
#include "controls/ctrlBaseTooltip.h"

struct MouseCoords;
class ITexture;

class ctrlImage : public Window, public ctrlBaseTooltip, public ctrlBaseImage
{
public:
    ctrlImage(Window* parent, unsigned id, const DrawPoint& pos, ITexture* image, const std::string& tooltip);
    ~ctrlImage() override;

    bool Msg_MouseMove(const MouseCoords& mc) override;
    Rect GetBoundaryRect() const override;

protected:
    void Draw_() override;
};
