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

#ifndef IRenderer_h__
#define IRenderer_h__

#include "DrawPoint.h"
#include "Rect.h"

class glArchivItem_Bitmap;

/// Render functions for basic stuff
/// Abstracts away the used algorithms
class IRenderer
{
protected:
    virtual ~IRenderer(){};

public:
    /// Draw a rect with 3D effect
    /// elevated: true for elevated, false for deepened effect
    /// borderImg: Texture to use for border
    /// contentImg: Texture to use for content or NULL to skip
    /// illuminated: Draw content illuminated
    /// contentColor: Color for the content
    virtual void DrawRect3D(const Rect& rect, bool elevated, glArchivItem_Bitmap& borderImg, glArchivItem_Bitmap* contentImg,
                            bool illuminated, unsigned contentColor) = 0;
    virtual void DrawRect(const Rect& rect, unsigned color) = 0;
    virtual void DrawLine(DrawPoint pt1, DrawPoint pt2, unsigned width, unsigned color) = 0;
};

#endif // IRenderer_h__
