// Copyright (c) 2005 - 2018 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "OpenGLRenderer.h"
#include "DrawPoint.h"
#include "glArchivItem_Bitmap.h"
#include "openglCfg.hpp"
#include <glad/glad.h>

void OpenGLRenderer::synchronize()
{
    glFinish();
}

void OpenGLRenderer::Draw3DBorder(const Rect& rect, bool elevated, glArchivItem_Bitmap& texture)
{
    const Extent rectSize = rect.getSize();
    if(rectSize.x < 4 || rectSize.y < 4)
        return;

    DrawPoint origin = rect.getOrigin();

    // Position of the horizontal and vertical image border
    DrawPoint horImgBorderPos(origin);
    DrawPoint vertImgBorderPos(origin);

    if(!elevated)
    {
        // For deepened effect the img border is at bottom and right
        // else it stays top and left
        horImgBorderPos += DrawPoint(0, rectSize.y - 2);
        vertImgBorderPos += DrawPoint(rectSize.x - 2, 0);
    }
    // Draw img borders
    texture.DrawPart(Rect(horImgBorderPos, Extent(rectSize.x, 2)));
    texture.DrawPart(Rect(vertImgBorderPos, Extent(2, rectSize.y)));

    // Draw black borders over the img borders
    glDisable(GL_TEXTURE_2D);
    glColor3f(0.0f, 0.0f, 0.0f);
    glBegin(GL_TRIANGLE_STRIP);
    // Left lower point
    DrawPoint lbPt = rect.getOrigin() + DrawPoint(rectSize);
    if(elevated)
    {
        // Bottom line with edge in left top and right line with little edge on left top
        glVertex2i(lbPt.x, origin.y);
        glVertex2i(lbPt.x - 2, origin.y + 1);
        glVertex2i(lbPt.x, lbPt.y);
        glVertex2i(lbPt.x - 2, lbPt.y - 2);
        glVertex2i(origin.x, lbPt.y);
        glVertex2i(origin.x + 1, lbPt.y - 2);
    } else
    {
        // Top line with edge on right and left line with edge on bottom
        glVertex2i(origin.x, lbPt.y);
        glVertex2i(origin.x + 2, lbPt.y - 1);
        glVertex2i(origin.x, origin.y);
        glVertex2i(origin.x + 2, origin.y + 2);
        glVertex2i(lbPt.x, origin.y);
        glVertex2i(lbPt.x - 1, origin.y + 2);
    }
    glEnd();
    glEnable(GL_TEXTURE_2D);
}

void OpenGLRenderer::Draw3DContent(const Rect& rect, bool elevated, glArchivItem_Bitmap& texture, bool illuminated,
                                   unsigned color)
{
    if(illuminated)
    {
        // Modulate2x anmachen
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
        glTexEnvf(GL_TEXTURE_ENV, GL_RGB_SCALE, 2.0f);
    }

    DrawPoint contentOffset(0, 0);
    if(elevated)
    {
        // Move a bit to left upper for elevated version
        contentOffset = DrawPoint(2, 2);
    }
    texture.DrawPart(rect, contentOffset, color);

    if(illuminated)
    {
        // Modulate2x wieder ausmachen
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    }
}

void OpenGLRenderer::DrawRect(const Rect& rect, unsigned color)
{
    glDisable(GL_TEXTURE_2D);

    glColor4ub(GetRed(color), GetGreen(color), GetBlue(color), GetAlpha(color));

    glBegin(GL_QUADS);
    glVertex2i(rect.left, rect.top);
    glVertex2i(rect.left, rect.bottom);
    glVertex2i(rect.right, rect.bottom);
    glVertex2i(rect.right, rect.top);
    glEnd();

    glEnable(GL_TEXTURE_2D);
}

void OpenGLRenderer::DrawLine(DrawPoint pt1, DrawPoint pt2, unsigned width, unsigned color)
{
    glDisable(GL_TEXTURE_2D);
    glColor4ub(GetRed(color), GetGreen(color), GetBlue(color), GetAlpha(color));

    glLineWidth(static_cast<GLfloat>(width));
    glBegin(GL_LINES);
    glVertex2i(pt1.x, pt1.y);
    glVertex2i(pt2.x, pt2.y);
    glEnd();

    glEnable(GL_TEXTURE_2D);
}

bool OpenGLRenderer::initOpenGL(OpenGL_Loader_Proc loader)
{
#if RTTR_OGL_ES
    return gladLoadGLES2Loader(loader) != 0;
#else
    return gladLoadGLLoader(loader) != 0;
#endif
}
