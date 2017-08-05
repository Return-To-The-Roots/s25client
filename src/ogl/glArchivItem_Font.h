// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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
#ifndef GLARCHIVITEM_FONT_H_INCLUDED
#define GLARCHIVITEM_FONT_H_INCLUDED

#pragma once

#include "ogl/glArchivItem_Bitmap.h"
#include "libutil/src/colors.h"
#include "ogl/oglIncludes.h"
#include "DrawPoint.h"
#include "Rect.h"
#include "helpers/containerUtils.h"
#include "libsiedler2/src/ArchivItem_Font.h"
#include "libutil/src/ucString.h"
#include <boost/smart_ptr/scoped_ptr.hpp>
#include <map>
#include <vector>
#include <string>

/// Klasse für GL-Fontfiles.
class glArchivItem_Font : public libsiedler2::ArchivItem_Font
{
    public:
        glArchivItem_Font() : ArchivItem_Font(), fontNoOutline(NULL), fontWithOutline(NULL) {}
        glArchivItem_Font(const glArchivItem_Font& item);

        /// Zeichnet einen Text.
        void Draw(DrawPoint pos, const ucString& wtext,   unsigned format, unsigned color = COLOR_WHITE, unsigned short length = 0, unsigned short max = 0xFFFF, const ucString& wend = cvWideStringToUnicode(L"..."));
        void Draw(DrawPoint pos, const std::string& text, unsigned format, unsigned color = COLOR_WHITE, unsigned short length = 0, unsigned short max = 0xFFFF, const std::string& end = "...");

        /// liefert die Länge einer Zeichenkette.
        unsigned short getWidth(const ucString& text, unsigned length = 0, unsigned max_width = 0xffffffff, unsigned* maxNumChars = NULL) const;
        unsigned short getWidth(const std::string& text, unsigned length = 0, unsigned max_width = 0xffffffff, unsigned* maxNumChars = NULL) const;
        /// liefert die Höhe des Textes ( entspricht @p getDy() )
        unsigned short getHeight() const { return dy + 1; }

        /// Return the bounds of the text when draw at the specified position with the specified format
        Rect getBounds(DrawPoint pos, const std::string& text, unsigned format) const;

        /// Gibt Infos, über die Unterbrechungspunkte in einem Text
        class WrapInfo
        {
            public:
                /// Erzeugt ein Arrays aus eigenständigen Strings aus den Unterbrechungsinfos.
                std::vector<std::string> CreateSingleStrings(const std::string& origin_text);

                /// Array von Positionen, wo der Text umbrochen werden soll (jeweils der Anfang vom String)
                std::vector<unsigned> positions;
        };

        /// Gibt Infos, über die Unterbrechungspunkte in einem Text, versucht Wörter nicht zu trennen, tut dies aber, falls
        /// es unumgänglich ist (Wort länger als die Zeile)
        WrapInfo GetWrapInfo(const std::string& text, const unsigned short primary_width, const unsigned short secondary_width);

        enum
        {
            DF_LEFT   = 0,
            DF_RIGHT  = 1,
            DF_CENTER = 2
        };

        enum
        {
            DF_TOP     = 0,
            DF_BOTTOM  = 4,
            DF_VCENTER = 8
        };

        enum
        {
            DF_NO_OUTLINE = 16
        };

        struct CharInfo
        {
            CharInfo() : pos(0, 0), width(0) {}
            CharInfo(const Position& pos, unsigned width): pos(pos), width(width){}
            Position pos;
            unsigned width;
        };

        /// prüft ob ein Buchstabe existiert.
        bool CharExist(unsigned c) const { return helpers::contains(utf8_mapping, c); }

        /// liefert die Breite eines Zeichens
        unsigned CharWidth(unsigned c) const { return GetCharInfo(c).width; }
        unsigned CharWidth(CharInfo ci) const { return ci.width; }

    private:

        struct GL_T2F_V3F_Struct
        {
            GLfloat tx, ty;
            GLfloat x, y, z;
        };

        void initFont();
        /// liefert das Char-Info eines Zeichens
        const CharInfo& GetCharInfo(unsigned c) const;
        void DrawChar(unsigned curChar, std::vector<GL_T2F_V3F_Struct>& vertices, DrawPoint& curPos, const Point<float>& texSize) const;

        boost::scoped_ptr<glArchivItem_Bitmap> fontNoOutline;
        boost::scoped_ptr<glArchivItem_Bitmap> fontWithOutline;

        std::map<unsigned, CharInfo> utf8_mapping;
        CharInfo placeHolder; /// Placeholder if glyph is missing

        /// Get width of the sequence defined by the begin/end pair of iterators (returning Unicode chars)
        /// The width will be at most maxWidth. The number of chars (or the iterator distance) is returned in maxNumChars (if specified)
        template<class T_Iterator>
        unsigned getWidthInternal(const T_Iterator& begin, const T_Iterator& end, unsigned maxWidth = 0xffffffff, unsigned* maxNumChars = NULL) const;
};

#endif // !GLARCHIVITEM_FONT_H_INCLUDED
