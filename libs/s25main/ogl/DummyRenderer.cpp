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

#include "DummyRenderer.h"
#include "openglCfg.hpp"
#include <s25util/warningSuppression.h>
#include <glad/glad.h>

namespace rttrOglMock {
RTTR_IGNORE_DIAGNOSTIC("-Wmissing-declarations")

void APIENTRY glGenTextures(GLsizei n, GLuint* textures)
{
    static GLuint cur = 0;
    for(; n > 0; --n)
        *(textures++) = ++cur;
}
void APIENTRY glDeleteTextures(GLsizei, const GLuint*) {}
void APIENTRY glBindTexture(GLenum, GLuint) {}
void APIENTRY glTexParameteri(GLenum, GLenum, GLint) {}
void APIENTRY glTexImage2D(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid*) {}
void APIENTRY glClear(GLbitfield) {}
void APIENTRY glVertexPointer(GLint, GLenum, GLsizei, const GLvoid*) {}
void APIENTRY glTexCoordPointer(GLint, GLenum, GLsizei, const GLvoid*) {}
void APIENTRY glColor4ub(GLubyte, GLubyte, GLubyte, GLubyte) {}
void APIENTRY glDrawArrays(GLenum, GLint, GLsizei) {}
void APIENTRY glGetTexLevelParameteriv(GLenum, GLint, GLenum, GLint* params)
{
    *params = 1;
}

RTTR_POP_DIAGNOSTIC
} // namespace rttrOglMock

bool DummyRenderer::initOpenGL(OpenGL_Loader_Proc)
{
    GLVersion = {RTTR_OGL_MAJOR, RTTR_OGL_MINOR};
#define MOCK(FUNC) FUNC = rttrOglMock::FUNC
    MOCK(glGenTextures);
    MOCK(glDeleteTextures);
    MOCK(glBindTexture);
    MOCK(glTexParameteri);
    MOCK(glTexImage2D);
    MOCK(glClear);
    MOCK(glVertexPointer);
    MOCK(glTexCoordPointer);
    MOCK(glColor4ub);
    MOCK(glDrawArrays);
    MOCK(glGetTexLevelParameteriv);
    return true;
}
