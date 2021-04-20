// Copyright (c) 2005 - 2018 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "DrawPoint.h"
#include "Rect.h"

class glArchivItem_Bitmap;

/// Render functions for basic stuff
/// Abstracts away the used algorithms
class IRenderer
{
public:
    /// Function type for loading OpenGL methods
    using OpenGL_Loader_Proc = void* (*)(const char*);

    virtual ~IRenderer() = default;
    virtual bool initOpenGL(OpenGL_Loader_Proc) = 0;
    /// Synchronize the rendering pipeline. Usually not required unless measuring something
    virtual void synchronize(){};
    /// Draw a border around rect with 3D effect
    /// @param elevated true for elevated, false for deepened effect
    /// @param texture Texture to use
    virtual void Draw3DBorder(const Rect& rect, bool elevated, glArchivItem_Bitmap& texture) = 0;
    /// Draw the content with 3D effect
    /// @param elevated true for elevated, false for deepened effect
    /// @param texture Texture to use
    /// @param illuminated Draw illuminated
    /// @param color Color for the content
    virtual void Draw3DContent(const Rect& rect, bool elevated, glArchivItem_Bitmap& texture, bool illuminated,
                               unsigned color) = 0;
    virtual void DrawRect(const Rect& rect, unsigned color) = 0;
    virtual void DrawLine(DrawPoint pt1, DrawPoint pt2, unsigned width, unsigned color) = 0;
};
