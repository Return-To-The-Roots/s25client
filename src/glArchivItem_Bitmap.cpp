// $Id: glArchivItem_Bitmap.cpp 7084 2011-03-26 21:31:12Z OLiver $
//
// Copyright (c) 2005 - 2010 Settlers Freaks (sf-team at siedler25.org)
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

///////////////////////////////////////////////////////////////////////////////
// Header
#include "stdafx.h"
#include "main.h"
#include "glArchivItem_Bitmap.h"

#include "VideoDriverWrapper.h"
#include "GlobalVars.h"
#include "Loader.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
	#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
	#undef THIS_FILE
	static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////
/** @class glArchivItem_Bitmap
 *
 *  Basisklasse f¸r GL-Bitmapitems.
 *
 *  @author FloSoft
 */

///////////////////////////////////////////////////////////////////////////////
/** @var glArchivItem_Bitmap::texture
 *
 *  OpenGL-Textur des Bildes.
 *
 *  @author FloSoft
 */

///////////////////////////////////////////////////////////////////////////////
/** 
 *  Konstruktor von @p glArchivItem_Bitmap.
 *
 *  @author FloSoft
 */
glArchivItem_Bitmap::glArchivItem_Bitmap(void)
	: baseArchivItem_Bitmap(), texture(0), filter(GL_NEAREST)
{
}

///////////////////////////////////////////////////////////////////////////////
/** 
 *  Kopiekonstruktor von @p glArchivItem_Bitmap.
 *
 *  @author FloSoft
 */
glArchivItem_Bitmap::glArchivItem_Bitmap(const glArchivItem_Bitmap *item)
	: baseArchivItem_Bitmap(item), texture(0), filter(GL_NEAREST)
{
}

///////////////////////////////////////////////////////////////////////////////
/** 
 *  Destruktor von @p glArchivItem_Bitmap.
 *
 *  @author FloSoft
 */
glArchivItem_Bitmap::~glArchivItem_Bitmap(void)
{
	DeleteTexture();
}

///////////////////////////////////////////////////////////////////////////////
/** 
 *  Zeichnet die Textur.
 *
 *  @author FloSoft
 */
void glArchivItem_Bitmap::Draw(short dst_x, short dst_y, short dst_w, short dst_h, short src_x, short src_y, short src_w, short src_h, const unsigned int color, const unsigned int unused)
{
	if(texture == 0)
		GenerateTexture();
	if(texture == 0)
		return;

	if(src_w == 0)
		src_w = width;
	if(src_h == 0)
		src_h = height;
	if(dst_w == 0)
		dst_w = src_w;
	if(dst_h == 0)
		dst_h = src_h;

	glEnable(GL_TEXTURE_2D);

	glColor4ub( GetRed(color), GetGreen(color), GetBlue(color), GetAlpha(color));
	glBindTexture(GL_TEXTURE_2D, texture);

	assert(getBobType() != libsiedler2::BOBTYPE_BITMAP_PLAYER);

	union sl {
		unsigned short s[4];
		uint64_t l;
	};

	sl dst;
	dst.s[0] = 0; //dst_x;
	dst.s[1] = 0; //dst_y;
	dst.s[2] = dst_w;
	dst.s[3] = dst_h;

	sl src;
	src.s[0] = src_x;
	src.s[1] = src_y;
	src.s[2] = src_w;
	src.s[3] = src_h;
		
	calllistmapmap::const_iterator mapmap = calllists.find(dst.l);
	if(mapmap != calllists.end())
	{
		calllistmap::const_iterator map = mapmap->second.find(src.l);
		if(map != mapmap->second.end() && glIsList(map->second))
		{
			glTranslatef((float)dst_x, (float)dst_y, 0);
			glCallList(map->second);
			glTranslatef((float)-dst_x, (float)-dst_y, 0);
			return;
		}
	}

	unsigned int list = glGenLists(1);

	/*std::cout << "generate  " << list << " for " 
		<< dst_x << "," << dst_y << "," << dst_w << "x" << dst_h << " and "
		<< src_x << "," << src_y << "," << src_w << "x" << src_h 
		<< std::endl;*/

	glNewList(list, GL_COMPILE);
	glBegin(GL_QUADS);
	DrawVertex( (float)(-nx),         (float)(-ny),         (float)src_x,         (float)src_y);
	DrawVertex( (float)(-nx),         (float)(-ny + dst_h), (float)src_x,         (float)(src_y+src_h));
	DrawVertex( (float)(-nx + dst_w), (float)(-ny + dst_h), (float)(src_x+src_w), (float)(src_y+src_h));
	DrawVertex( (float)(-nx + dst_w), (float)(-ny),         (float)(src_x+src_w), (float)src_y);
	glEnd();
	glEndList();

	// should be faster
	glTranslatef((float)dst_x, (float)dst_y, 0);
	glCallList(list);
	glTranslatef((float)-dst_x, (float)-dst_y, 0);

	calllists[dst.l][src.l] = list;
}

///////////////////////////////////////////////////////////////////////////////
/** 
 *  Liefert das GL-Textur-Handle.
 *
 *  @author FloSoft
 */
unsigned int glArchivItem_Bitmap::GetTexture()
{
	if(texture == 0)
		GenerateTexture();
	return texture;
}

///////////////////////////////////////////////////////////////////////////////
/** 
 *  Lˆscht die GL-Textur (z.B f¸rs Neuerstellen)
 *
 *  @author FloSoft
 */
void glArchivItem_Bitmap::DeleteTexture()
{
	glDeleteTextures(1, (const GLuint*)&texture);
	texture = 0;
}

///////////////////////////////////////////////////////////////////////////////
/** 
 *  Setzt den Texturfilter auf einen bestimmten Wert.
 *
 *  @author FloSoft
 */
void glArchivItem_Bitmap::setFilter(unsigned int filter)
{
	if(this->filter == filter)
		return;

	this->filter = filter;

	// neugenerierung der Textur anstoﬂen
	if(texture != 0)
		DeleteTexture();
}

///////////////////////////////////////////////////////////////////////////////
/** 
 *  Erzeugt die Textur.
 *
 *  @author FloSoft
 */
void glArchivItem_Bitmap::GenerateTexture(void)
{
	texture = VideoDriverWrapper::inst().GenerateTexture();

	if(!palette)
		setPalette(LOADER.GetPaletteN("pal5"));
	
	glEnable(GL_TEXTURE_2D);

	glBindTexture(GL_TEXTURE_2D, texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);

	int iformat = GL_RGBA, dformat = GL_BGRA;

	unsigned char *buffer = new unsigned char[tex_width*tex_height*4];

	memset(buffer, 0, tex_width*tex_height*4);
	print(buffer, tex_width, tex_height, libsiedler2::FORMAT_RGBA, palette, 0, 0, 0, 0, 0, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, iformat, tex_width, tex_height, 0, dformat, GL_UNSIGNED_BYTE, buffer);

	delete[] buffer;
}

///////////////////////////////////////////////////////////////////////////////
/** 
 *  Zeichnet einen Vertex inkl Texturkoordinaten.
 *
 *  @author FloSoft
 */
/*void glArchivItem_Bitmap::DrawVertex(float x, float y, float tx, float ty)
{
	glTexCoord2f(tx/tex_width, ty/tex_height);
	glVertex2f(x, y);
}*/
