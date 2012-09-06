// $Id: glBitmap.cpp 8155 2012-09-06 02:11:55Z Maqs $
//
// Copyright (c) 2005 - 2012 Settlers Freaks (sf-team at siedler25.org)
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

#ifndef GLBITMAP_H_INCLUDED
#define GLBITMAP_H_INCLUDED

#pragma once

#include <vector>

#include "glArchivItem_Bitmap.h"
#include "glArchivItem_Bitmap_Player.h"

enum glBitmapItemType
{
	TYPE_ARCHIVITEM_BITMAP = 0,
	TYPE_ARCHIVITEM_BITMAP_PLAYER,
	TYPE_ARCHIVITEM_BITMAP_SHADOW
};

class glBitmapItem
{
public:
	glBitmapItem(glArchivItem_Bitmap *b, bool shadow = false) {bmp = b; type = shadow ? TYPE_ARCHIVITEM_BITMAP_SHADOW : TYPE_ARCHIVITEM_BITMAP; nx = b->getNx(); ny = b->getNy(); w = b->getWidth(); h = b->getHeight();}
	glBitmapItem(glArchivItem_Bitmap_Player *b) {bmp = b; type = TYPE_ARCHIVITEM_BITMAP_PLAYER; nx = b->getNx(); ny = b->getNy(); w = b->getWidth(); h = b->getHeight();}

	void *bmp;
	glBitmapItemType type;

	int nx, ny;
	int w, h;
};

class glSmartBitmap
{
private:
	int w, h;
	int nx, ny;

	unsigned int texture;

	bool hasPlayer;


	struct GL_T2F_C4UB_V3F_Struct
	{
		GLfloat tx, ty;
		GLubyte r, g, b, a;
		GLfloat x, y, z;
	};

	GL_T2F_C4UB_V3F_Struct tmp[8];

	std::vector<glBitmapItem> items;

public:
	glSmartBitmap() : w(0), h(0), nx(0), ny(0), texture(0), hasPlayer(false) {}
	~glSmartBitmap() {if (texture) glDeleteTextures(1, &texture);}

	inline bool isGenerated() {return(texture != 0);}

	unsigned nextPowerOfTwo(unsigned k);
	void generateTexture();
	void draw(int x, int y, unsigned player_color = 0x00000000);

	void add(glArchivItem_Bitmap *bmp) {if (bmp) items.push_back(glBitmapItem(bmp));}
	void add(glArchivItem_Bitmap_Player *bmp) {if (bmp) items.push_back(glBitmapItem(bmp));}
	void addShadow(glArchivItem_Bitmap *bmp) {if (bmp) items.push_back(glBitmapItem(bmp, true));}
};

#endif

