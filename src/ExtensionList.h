// $Id: ExtensionList.h 9357 2014-04-25 15:35:25Z FloSoft $
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
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
#ifndef EXTENSIONLIST_H_INCLUDED
#define EXTENSIONLIST_H_INCLUDED

#pragma once

#ifdef _WIN32
#include <windows.h>
#endif // _WIN32

#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>

#define GL_COMBINE_EXT                    0x8570
#define GL_RGB_SCALE_EXT                  0x8573
#else
#include <GL/gl.h>
#include <GL/glext.h>
#endif // !__APPLE__

// WGL_EXT_swap_control
#ifdef _WIN32
typedef BOOL (APIENTRY* PFNWGLSWAPINTERVALFARPROC)(int);
#else
typedef int (*PFNWGLSWAPINTERVALFARPROC)(int);
#endif

extern PFNWGLSWAPINTERVALFARPROC wglSwapIntervalEXT;

#ifndef __APPLE__
// GL_ARB_vertex_buffer_object
extern PFNGLBINDBUFFERARBPROC glBindBufferARB; // VBO Bind-Prozedur
extern PFNGLDELETEBUFFERSARBPROC glDeleteBuffersARB; // VBO Lösch-Prozedur
extern PFNGLGENBUFFERSARBPROC glGenBuffersARB; // VBO Namens Generations-Prozedur
extern PFNGLBUFFERDATAARBPROC glBufferDataARB; // VBO Daten-Lade-Prozedur
extern PFNGLBUFFERSUBDATAARBPROC glBufferSubDataARB; /// VBO Daten-Änder-Prozedur
// GL_EXT_paletted_texture
extern PFNGLCOLORTABLEEXTPROC glColorTableEXT;
#endif // !__APPLE__

#endif // EXTENSIONLIST_H_INCLUDED
