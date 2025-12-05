// Copyright (C) 2005 - 2025 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "controls/ctrlDeepening.h"
#include "ctrlBaseImage.h"

/// Image Deepening
class ctrlImageDeepening : public ctrlDeepening, public ctrlBaseImage
{
public:
    ctrlImageDeepening(Window* parent, unsigned id, DrawPoint pos, const Extent& size, TextureColor tc,
                       ITexture* image);

protected:
    void DrawContent() const override;
};
