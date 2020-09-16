// Copyright (c) 2005 - 2018 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Return To The Roots.
//
// Return To The Roots is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Return To The Roots is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Return To The Roots. If not, see <http://www.gnu.org/licenses/>.

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
