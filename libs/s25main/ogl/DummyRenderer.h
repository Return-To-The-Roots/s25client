// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "IRenderer.h"

class glArchivItem_Bitmap;

class DummyRenderer : public IRenderer
{
public:
    bool initOpenGL(OpenGL_Loader_Proc) override;
    void draw3DBorder(const Rect&, bool /*elevated*/, glArchivItem_Bitmap& /*texture*/) override {}
    void draw3DContent(const Rect&, bool /*elevated*/, glArchivItem_Bitmap& /*texture*/, bool /*illuminated*/,
                       unsigned /*color*/) override
    {}
    void drawRect(const Rect&, unsigned) override {}
    void drawLine(DrawPoint, DrawPoint, unsigned, unsigned) override {}
};
