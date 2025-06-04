// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "DummyRenderer.h"
#include <s25util/warningSuppression.h>
#ifndef __EMSCRIPTEN__
#include "openglCfg.hpp"
#include <glad/glad.h>
#endif

namespace rttrOglMock {
RTTR_IGNORE_DIAGNOSTIC("-Wmissing-declarations")
#ifndef __EMSCRIPTEN__
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
#endif

RTTR_POP_DIAGNOSTIC
} // namespace rttrOglMock

bool DummyRenderer::initOpenGL(OpenGL_Loader_Proc)
{
#ifndef __EMSCRIPTEN__
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
#endif
    return true;
}
