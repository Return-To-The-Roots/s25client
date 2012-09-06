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

///////////////////////////////////////////////////////////////////////////////
// Header
#include "main.h"
#include "glSmartBitmap.h"

#include <climits>

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
	#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
	#undef THIS_FILE
	static char THIS_FILE[] = __FILE__;
#endif

unsigned glSmartBitmap::nextPowerOfTwo(unsigned k)
{
	if (k == 0)
	{
		return(1);
	}

	k--;

	for (unsigned i = 1; i < sizeof(unsigned)*CHAR_BIT; i <<= 1)
	{
		k = k | k >> i;
	}

	return(k + 1);
}

void glSmartBitmap::generateTexture()
{
	if (items.empty())
	{
		return;
	}

	int max_x = 0;
	int max_y = 0;

	nx = ny = INT_MIN;

	if (!texture)
	{
		glGenTextures(1, &texture);

		if (!texture)
		{
			return;
		}
	}

	hasPlayer = false;

	for (std::vector<glBitmapItem>::const_iterator it = items.begin(); it != items.end(); ++it)
	{
		if ((*it).type == TYPE_ARCHIVITEM_BITMAP_PLAYER)
		{
			hasPlayer = true;
		}

		if (nx < (*it).nx)
		{
			nx = (*it).nx;
		}

		if (ny < (*it).ny)
		{
			ny = (*it).ny;
		}

		if (max_x < (*it).w - (*it).nx)
		{
			max_x = (*it).w - (*it).nx;
		}

		if (max_y < (*it).h - (*it).ny)
		{
			max_y = (*it).h - (*it).ny;
		}
	}

	w = nextPowerOfTwo(nx + max_x);
	h = nextPowerOfTwo(ny + max_y);

	// do we have a player-colored overlay?
	unsigned stride = hasPlayer ? w * 2 : w;

	unsigned char *buffer = new unsigned char[stride * h * 4];
	memset(buffer, 0, stride * h * 4);

	libsiedler2::ArchivItem_Palette *p_colors = LOADER.GetPaletteN("colors");
	libsiedler2::ArchivItem_Palette *p_5 = LOADER.GetPaletteN("pal5");

	for (std::vector<glBitmapItem>::const_iterator it = items.begin(); it != items.end(); ++it)
	{
		unsigned xo = (nx - (*it).nx);
		unsigned yo = (ny - (*it).ny);

		switch ((*it).type)
		{
			case TYPE_ARCHIVITEM_BITMAP:
			{
				glArchivItem_Bitmap *bmp = static_cast<glArchivItem_Bitmap *>((*it).bmp);

				bmp->print(buffer, stride, h, libsiedler2::FORMAT_RGBA, p_5, xo, yo, 0, 0, 0, 0);

				break;
			}
			case TYPE_ARCHIVITEM_BITMAP_PLAYER:
			{
				glArchivItem_Bitmap_Player *bmp = static_cast<glArchivItem_Bitmap_Player *>((*it).bmp);

				if ((xo + bmp->getWidth() > w) || (yo + bmp->getHeight() > h))
				{
					fprintf(stderr, "%u,%u (%ux%u) in %u,%u\n", xo, yo, bmp->getWidth(), bmp->getHeight(), w, h);
				}

				bmp->print(buffer, stride, h, libsiedler2::FORMAT_RGBA, p_colors, 128,
					xo, yo, 0, 0, 0, 0, false);

				bmp->print(buffer, stride, h, libsiedler2::FORMAT_RGBA, p_colors, 128,
					xo + w, yo, 0, 0, 0, 0, true);

				break;
			}

			case TYPE_ARCHIVITEM_BITMAP_SHADOW:
			{
				glArchivItem_Bitmap *bmp = static_cast<glArchivItem_Bitmap *>((*it).bmp);

				unsigned char *tmp = new unsigned char[w * h * 4];
				memset(tmp, 0, w * h * 4);

				bmp->print(tmp, w, h, libsiedler2::FORMAT_RGBA, p_5, xo, yo, 0, 0, 0, 0);

				unsigned idx = 0;
				for (unsigned x = 0; x < w; ++x)
				{
					for (unsigned y = 0; y < h; ++y)
					{
						if (tmp[idx + 3] != 0x00)
						{
							buffer[idx + 0] = 0x00;
							buffer[idx + 1] = 0x00;
							buffer[idx + 2] = 0x00;
							buffer[idx + 3] = 0x40;
						}

						idx += 4;
					}
				}

				delete[] tmp;

				break;
			}
			default:
				break;
		}
	}

	glBindTexture(GL_TEXTURE_2D, texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, stride, h, 0, GL_BGRA, GL_UNSIGNED_BYTE, buffer);

	delete[] buffer;

	tmp[0].z = tmp[1].z = tmp[2].z = tmp[3].z = 0.0;

	tmp[0].tx = tmp[1].tx = 0.0;
	tmp[2].tx = tmp[3].tx = hasPlayer ? 0.5 : 1.0;

	tmp[0].ty = tmp[3].ty = 0.0;
	tmp[1].ty = tmp[2].ty = 1.0;

	tmp[0].r = tmp[1].r = tmp[2].r = tmp[3].r = 0xFF;
	tmp[0].g = tmp[1].g = tmp[2].g = tmp[3].g = 0xFF;
	tmp[0].b = tmp[1].b = tmp[2].b = tmp[3].b = 0xFF;
	tmp[0].a = tmp[1].a = tmp[2].a = tmp[3].a = 0xFF;

	tmp[4].tx = tmp[5].tx = 0.5;
	tmp[6].tx = tmp[7].tx = 1.0;
}

void glSmartBitmap::draw(int x, int y, unsigned player_color)
{
	bool player = false;

	if (!texture)
		generateTexture();
	if (!texture)
		return;

	if ((GetAlpha(player_color) != 0x00) && hasPlayer)
	{
		player = true;
	}

	tmp[0].x = tmp[1].x = x - nx;
	tmp[2].x = tmp[3].x = x - nx + w;

	tmp[0].y = tmp[3].y = y - ny;
	tmp[1].y = tmp[2].y = y - ny + h;


	if (player)
	{
		tmp[4].r = tmp[5].r = tmp[6].r = tmp[7].r = GetRed(player_color);
		tmp[4].g = tmp[5].g = tmp[6].g = tmp[7].g = GetGreen(player_color);
		tmp[4].b = tmp[5].b = tmp[6].b = tmp[7].b = GetBlue(player_color);
		tmp[4].a = tmp[5].a = tmp[6].a = tmp[7].a = GetAlpha(player_color);
	}

	glInterleavedArrays(GL_T2F_C4UB_V3F, 0, tmp);

	glBindTexture(GL_TEXTURE_2D, texture);

	glDrawArrays(GL_QUADS, 0, player ? 8 : 4);
}

