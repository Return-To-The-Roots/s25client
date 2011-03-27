// $Id: glArchivItem_Font.cpp 7091 2011-03-27 10:57:38Z OLiver $
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
#include <stdafx.h>
#include "main.h"
#include "glArchivItem_Font.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
	#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
	#undef THIS_FILE
	static char THIS_FILE[] = __FILE__;
#endif


/// Ansi --> Oem konvertieren
const unsigned char ANSI_TO_OEM[256] =
{
	/*       0    1    2    3     4    5    6    7     8    9    A    B     C    D    E    F  */
	/* 0 */ 0x00,0x01,0x02,0x03, 0x04,0x05,0x06,0x07, 0x08,0x09,0x0a,0x0b, 0x0c,0xd0,0x0e,0x0f,
	/* 1 */ 0x10,0x11,0x12,0x13, 0x14,0x15,0x16,0x17, 0x18,0x19,0x1a,0x1b, 0x1c,0x1d,0x1e,0x1f, 
	/* 2 */ 0x20,0x21,0x22,0x23, 0x24,0x25,0x26,0x27, 0x28,0x29,0x2a,0x2b, 0x2c,0x2d,0x2e,0x2f, 
	/* 3 */ 0x30,0x31,0x32,0x33, 0x34,0x35,0x36,0x37, 0x38,0x39,0x3a,0x3b, 0x3c,0x3d,0x3e,0x3f, 
	/* 4 */ 0x40,0x41,0x42,0x43, 0x44,0x45,0x46,0x47, 0x48,0x49,0x4a,0x4b, 0x4c,0x4d,0x4e,0x4f, 
	/* 5 */ 0x50,0x51,0x52,0x53, 0x54,0x55,0x56,0x57, 0x58,0x59,0x5a,0x5b, 0x5c,0x5d,0x5f,0x5e, 
	/* 6 */ 0x60,0x61,0x62,0x63, 0x64,0x65,0x66,0x67, 0x68,0x69,0x6a,0x6b, 0x6c,0x6d,0x6e,0x6f, 
	/* 7 */ 0x70,0x71,0x72,0x73, 0x74,0x75,0x76,0x77, 0x78,0x79,0x7a,0x7b, 0x7c,0x7d,0x7e,0x7f, 

	/*       0    1    2    3     4    5    6    7     8    9    A    B     C    D    E    F  */
	/* 8 */ 0x00,0x00,0x00,0x9f, 0x00,0x00,0x00,0xd8, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,
	/* 9 */ 0x00,0x60,0x27,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,
	/* A */ 0xff,0xad,0x9b,0x9c, 0x0f,0x9d,0x7c,0x15, 0x22,0x63,0xa6,0xae, 0xaa,0x2d,0x52,0x00,
	/* B */ 0xf8,0xf1,0xfd,0x33, 0x27,0xe6,0x14,0xfa, 0x2c,0x31,0xa7,0xaf, 0xac,0xab,0x00,0xa8,
	/* C */ 0x41,0x41,0x41,0x41, 0x8e,0x8f,0x92,0x80, 0x45,0x90,0x45,0x45, 0x49,0x49,0x49,0x49,
	/* D */ 0x44,0xa5,0x4f,0x4f, 0x4f,0x4f,0x99,0x78, 0x4f,0x55,0x55,0x55, 0x9a,0x59,0x00,0xe1,
	/* E */ 0x85,0xa0,0x83,0x61, 0x84,0x86,0x91,0x87, 0x8a,0x82,0x88,0x89, 0x8d,0xa1,0x8c,0x8b,
	/* F */ 0x64,0xa4,0x95,0xa2, 0x93,0x6f,0x94,0xf6, 0x6f,0x97,0xa3,0x96, 0x81,0x79,0x00,0x98,
};


///////////////////////////////////////////////////////////////////////////////
/**
 *  Zeichnet einen Text.
 *
 *  @param[in] x      X-Koordinate
 *  @param[in] y      Y-Koordinate
 *  @param[in] text   Der Text
 *  @param[in] format Format des Textes (verodern)
 *                      @p glArchivItem_Font::DF_LEFT    - Text links ( standard )
 *                      @p glArchivItem_Font::DF_CENTER  - Text mittig
 *                      @p glArchivItem_Font::DF_RIGHT   - Text rechts
 *                      @p glArchivItem_Font::DF_TOP     - Text oben ( standard )
 *                      @p glArchivItem_Font::DF_VCENTER - Text vertikal zentriert
 *                      @p glArchivItem_Font::DF_BOTTOM  - Text unten
 *  @param[in] color  Farbe des Textes
 *  @param[in] length Länge des Textes
 *  @param[in] max    maximale Länge
 *
 *  @author OLiver
 */
void glArchivItem_Font::Draw(short x, 
							 short y, 
							 const std::string& text, 
							 unsigned int format, 
							 unsigned int color, 
							 unsigned short length, 
							 unsigned short max, 
							 const std::string& end, 
							 unsigned short end_length)
{
	if(!_font)
		initFont();

	// Breite bestimmen
	if(length == 0)
		length = (unsigned short)text.length();
	if(end_length == 0)
		end_length = (unsigned short)end.length();

	bool enable_end;

	unsigned short text_max = 0;
	unsigned short text_width = getWidth(text, length, max, &text_max);

	// text_max ist die maximale Indexzahl, nicht mehr die Breite in Pixel!
	if(end.length() && text_max < length)
	{
		unsigned short end_max = 0;
		unsigned short end_width = getWidth(end, end_length, max, &end_max);
		
		// Bei Überlauf oder kleiner 0 - nix zeichnen ;)
		if( text_width - end_width > text_width || text_width - end_width <= 0 )
			return;

		text_width -= end_width;

		// Wieviele Buchstaben gehen in den "Rest" (ohne "end")
		text_max = 0;
		getWidth(text, length, text_width, &text_max);

		enable_end = true;
	}
	else
		enable_end = false;

	if(text_max == 0)
		return;

	if( (format & 3) == DF_RIGHT)
		x -= text_width;
	if( (format & 12) == DF_BOTTOM)
		y -= dy;
	if( (format & 12) == DF_VCENTER)
		y -= dy / 2;

	short cx = x, cy = y;
	if( (format & 3) == DF_CENTER)
	{
		unsigned short line_width = text_width;
		for(unsigned short j = 0; j < text_max; ++j)
		{
			if(text[j] == '\n')
			{
				line_width = getWidth(text, j);
				break;
			}
		}
		cx = x - line_width / 2;
	}

	for(unsigned short i = 0; i < text_max; ++i)
	{
		if(text[i] == '\n')
		{
			cy += dy;
			if( (format & 3) == DF_CENTER)
			{
				unsigned short line_width = 0;
				for(unsigned short j = i+1; j < text_max; ++j)
				{
					if(text[j] == '\n')
					{
						line_width = getWidth(&text[i+1], j-i-1);
						break;
					}
				}
				if(line_width == 0)
					line_width = getWidth(&text[i+1], text_max-i-1);
				cx = x - line_width / 2;
			}
			else
				cx = x;
		}
		else
		{
			unsigned char c = ANSI_TO_OEM[(unsigned char)text[i]];
			if(_charwidths[c] > 0)
			{
				unsigned int x = 0, y = 0;
				x = c % 16;
				y = c / 16;

				_font->Draw(cx, cy, _charwidths[c], dy, x*(dx+2)+1, y*(dy+2)+1, _charwidths[c], dy, (GetAlpha(color) << 24) | 0x00FFFFFF, color);
				cx += _charwidths[c];
			}
		}
	}
	if(enable_end)
	{
		for(unsigned short i = 0; i < end_length; ++i)
		{
			if(end[i] == '\n')
			{
				cy += dy;
				cx = x;
			}
			else
			{
				unsigned char c = ANSI_TO_OEM[(unsigned char)end[i]];
				if(_charwidths[c] > 0)
				{
					unsigned int x = 0, y = 0;
					x = c % 16;
					y = c / 16;

					_font->Draw(cx, cy, 0, 0, x*(dx+2)+1, y*(dy+2)+1, _charwidths[c], dy, (GetAlpha(color) << 24) | 0x00FFFFFF, color);
					cx += _charwidths[c];
				}
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  prüft ob ein Buchstabe existiert.
 *
 *  @param[in] c Buchstabe der geprüft werden soll
 *
 *  @return @p true falls Buchstabe existiert, @p false wenn nicht
 *
 *  @author OLiver
 */
bool glArchivItem_Font::CharExist(unsigned char c) const
{
	return (_charwidths[ANSI_TO_OEM[c]] > 0);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  liefert die Länge einer Zeichenkette.
 *
 *  @param[in]     text   Der Text
 *  @param[in]     length Textlänge
 *  @param[in,out] max    In:  maximale Breite des Textes in Pixeln
 *                        Out: maximale Breite in Buchstaben der in "max"-Pixel reinpasst.
 *
 *  @return Breite des Textes in Pixeln
 *
 *  @author OLiver
 */
unsigned short glArchivItem_Font::getWidth(const std::string& text, unsigned length, unsigned max_width, unsigned short *max) const
{
	if(length == 0)
		length = unsigned(text.length());

	unsigned short w = 0, wm = 0;
	for(unsigned short i = 0; i < length; ++i)
	{
		unsigned short cw = _charwidths[ANSI_TO_OEM[(unsigned char)text[i]]];

		if(text[i] == '\n')
		{
			if(w > wm) // Längste Zeile
				wm = w;
			w = 0;
		}

		// haben wir das maximum erreicht?
		if(unsigned((wm > 0 ? wm : w) + cw) > max_width)
		{
			*max = i;
			if(wm == 0)
				wm = w;
			return wm;
		}
		w += cw;
	}

	if((wm == 0) || (wm < w)) // Letzte Zeile kann auch die längste sein und hat kein \n am Ende
		wm = w;

	if(max)
		*max = length;

	return wm;
}
void glArchivItem_Font::WrapInfo::CreateSingleStrings(const std::string& origin_text,std::string * dest_strings)
{
	// Kopie des ursprünglichen Strings erstellen
	std::string copy(origin_text);

	for(unsigned i = 0; i < positions.size(); ++i)
	{
		// Gibts noch weitere Teile danach?
		char temp = 0; 
		if(i + 1 < positions.size())
		{
			// dann muss statt des Leerzeichens o.Ä. ein Nullzeichen gesetzt werden, damit nur der Teilstring aufgenommen
			// wird und nicht noch alles was danach kommt

			// das Zeichen merken, was da vorher war
			temp = origin_text[positions.at(i+1)];
			// Zeichen 0 setzen
			copy[positions.at(i+1)] = 0;
		}

		dest_strings[i] = &copy[positions.at(i)];

		// wieder ggf. zurücksetzen, siehe oben
		if(i + 1 < positions.size())
			copy[positions.at(i+1)] = temp;
	}
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Gibt Infos, über die Unterbrechungspunkte in einem Text 
 *
 *  @param[in]     text            Text, der auf Zeilen verteilt werden soll
 *  @param[in]     primary_width   Maximale Breite der ersten Zeile
 *  @param[in]     secondary_width Maximale Breite der weiteren Zeilen
 *  @param[in,out] wi              Pointer auf die Anfänge der einzelnen Zeilen
 *
 *
 *  @author OLiver
 */
void glArchivItem_Font::GetWrapInfo(const std::string& text, 
									const unsigned short primary_width, const unsigned short secondary_width,
									WrapInfo& wi)
{
	if(!_font)
		initFont();
	// Breite der aktuellen Zeile
	unsigned short line_width = 0;
	// Breite des aktuellen Wortes
	unsigned short word_width = 0;
	// Beginn des aktuellen Wortes
	unsigned word_start = 0;

	// Logischerweise fangen wir in der ersten Zeile an
	wi.positions.clear();
	wi.positions.push_back(0);

	// Länge des Strings
	unsigned length = unsigned(text.length());

	for(unsigned i = 0; i <= length; ++i)
	{
		// Leerzeichen, Umbruch oder ende?
		if(text[i] == '\n' || text[i] == ' ' || i == length)
		{
			// Passt das letzte Wort mit auf die Zeile? (bzw bei newline immer neue zeile anfangen)
			if(word_width + line_width <= ( (wi.positions.size() == 1) ? primary_width : secondary_width))
			{
				// Länge des Leerzeichens mit draufaddieren
				line_width += word_width;
				line_width += _charwidths[(unsigned int)' '];

				// neues Wort fängt dann nach dem Leerzeichen an (falls wir nicht schon am Ende vom Text sind)
				if(i < length-1)
				{
					word_width = 0;
					word_start = i+1;
				}
			}
			else
			{
				// Ansonsten neue Zeile anfangen

				// Passt das Wort wenigsens komplett überhaupt in die nächste Zeile?
				if(word_width <= secondary_width)
				{
					// neue Zeile anfangen mit diesem Wort
					wi.positions.push_back(word_start);
					// In der Zeile ist schon das Wort und das jetzige Leerzeichen mit drin
					line_width = word_width + _charwidths[(unsigned int)' '];
					// Neues Wort beginnen (falls wir nicht schon am Ende vom Text sind)
					if(i < length-1)
					{
						word_start = i+1;
						word_width = 0;
					}
				}
				else
				{
					// ansonsten muss das Wort zwangsläufig auf mehrere Zeilen verteilt werden
					for(size_t z = 0; text[word_start+z] != ' ' && text[word_start+z]; ++z)
					{
						unsigned short letter_width = _charwidths[ANSI_TO_OEM[(unsigned char)text[word_start+z]]];

						// passt der neue Buchstabe noch mit drauf?
						if(line_width + letter_width <= ( (wi.positions.size() == 1) ? primary_width : secondary_width))
							line_width += letter_width;
						else
						{
							// wenn nicht, muss hier ein Umbruch erfolgen

							// neue Zeile anfangen mit diesem Buchstaben
							wi.positions.push_back(word_start+z);
							line_width = letter_width;
						}
					}

					// Leerzeichen nicht vergessen
					line_width += _charwidths[(unsigned int)' '];

					// Neues Wort beginnen (falls wir nicht schon am Ende vom Text sind)
					if(i < length-1)
					{
						word_start = i+1;
						word_width = 0;
					}
				}
			}
			// Bei Newline immer neue Zeile anfangen, aber erst jetzt
			// und nicht schon oben in diesem if-Zweig
			if(text[i] == '\n' && i < length-1)
			{
				word_start = i+1;
				word_width = 0;
				line_width = 0;
				wi.positions.push_back(word_start);
			}
		}
		else
		{
			// Anderes Zeichen --> einfach dessen Breite mit addieren
			if( CharExist((unsigned char)text[i]) )
				word_width += _charwidths[ANSI_TO_OEM[(unsigned char)text[i]]];
		}
	}

	// Ignore trailing newline
	if(wi.positions[wi.positions.size()-1] == length-1)
		wi.positions.pop_back();
}

void glArchivItem_Font::initFont()
{
	_font = dynamic_cast<glArchivItem_Bitmap_Player *>(glAllocator(libsiedler2::BOBTYPE_BITMAP_PLAYER, 0, NULL));

	memset(_charwidths, 0, sizeof(_charwidths));

	int w = (dx+2) * 16 + 2;
	int h = (dy+2) * 16 + 2;
	unsigned int buffersize = w * h * 4; // RGBA Puffer für alle Buchstaben
	unsigned char *buffer = new unsigned char[buffersize];
	memset(buffer, 0, buffersize);

	int x = 1;
	int y = 1 + 2 * (dy+2);
	for(int i = 32; i < 256; ++i)
	{
		if( (i % 16) == 0 && i != 32 )
		{
			y += dy+2;
			x = 1;
		}

		const glArchivItem_Bitmap_Player *c = dynamic_cast<const glArchivItem_Bitmap_Player *>(get(i));
		if(c)
		{
			// Spezialpalette (blaue Spielerfarben sind Grau) verwenden,
			// damit man per OpenGL einfärben kann!
			c->print(buffer, w, h, libsiedler2::FORMAT_RGBA, LOADER.GetPaletteN("colors"), 128, x, y);
			_charwidths[i] = c->getWidth();
		}
		x += dx+2;
	}

	// Spezialpalette (blaue Spielerfarben sind Grau) verwenden,
	// damit man per OpenGL einfärben kann!
	_font->create(w, h, buffer, w, h, libsiedler2::FORMAT_RGBA, LOADER.GetPaletteN("colors"), 128);
	_font->setFilter(GL_LINEAR);
}
