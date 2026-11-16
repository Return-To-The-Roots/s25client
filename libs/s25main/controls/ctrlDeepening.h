// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
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
    void drawImpl() override;
    /// Derived classes have to draw the content
    virtual void drawContent() const = 0;

private:
    TextureColor tc;
};
