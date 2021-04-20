// Copyright (c) 2005 - 2018 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "IRenderer.h"

class glArchivItem_Bitmap;

class DummyRenderer : public IRenderer
{
public:
    bool initOpenGL(OpenGL_Loader_Proc) override;
    void Draw3DBorder(const Rect&, bool /*elevated*/, glArchivItem_Bitmap& /*texture*/) override {}
    void Draw3DContent(const Rect&, bool /*elevated*/, glArchivItem_Bitmap& /*texture*/, bool /*illuminated*/,
                       unsigned /*color*/) override
    {}
    void DrawRect(const Rect&, unsigned) override {}
    void DrawLine(DrawPoint, DrawPoint, unsigned, unsigned) override {}
};
