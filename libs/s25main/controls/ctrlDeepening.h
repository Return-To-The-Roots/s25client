// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "DrawPoint.h"
#include "Window.h"

/// Control with a "deepened" look by a 3D border
class ctrlDeepening : public Window
{
public:
    static constexpr DrawPoint borderSize{2, 2};

    ctrlDeepening(Window* parent, unsigned id, DrawPoint pos, const Extent& size, TextureColor tc);

protected:
    void Draw_() override;
    /// Derived classes have to draw the content
    virtual void DrawContent() const = 0;

private:
    TextureColor tc;
};
