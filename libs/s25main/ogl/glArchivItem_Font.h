// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#include "DrawPoint.h"
#include "Rect.h"
#include "ogl/FontStyle.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "libsiedler2/ArchivItem_Font.h"
#include "s25util/colors.h"
#include <glad/glad.h>
#include <array>
#include <map>
#include <memory>
#include <string>
#include <vector>

/// Klasse für GL-Fontfiles.
class glArchivItem_Font : public libsiedler2::ArchivItem_Font //-V690
{
public:
    glArchivItem_Font();
    glArchivItem_Font(const glArchivItem_Font& obj);
    RTTR_CLONEABLE(glArchivItem_Font)

    /// Draw the the text at the given position with format (alignment) and color.
    /// If length is given, only that many chars (not glyphs!) will be used
    /// If maxWidth is given then the text length will be at most maxWidth. If the text is shortened then end is appended (included in
    /// maxWidth)
    void Draw(DrawPoint pos, const std::string& text, FontStyle format, unsigned color = COLOR_WHITE, unsigned short length = 0,
              unsigned short maxWidth = 0xFFFF, const std::string& end = "...");

    /// Return the width of the drawn text. If maxWidth is given then the width will be <= maxWidth and maxNumChars will be set to the
    /// maximum number of chars (not glyphs!) that fit into the width
    unsigned short getWidth(const std::string& text, unsigned length = 0) const;
    unsigned short getWidth(const std::string& text, unsigned length, unsigned maxWidth, unsigned* maxNumChars) const;
    /// liefert die Höhe des Textes ( entspricht @p getDy() )
    unsigned short getHeight() const { return dy + 1; }

    /// Return the bounds of the text when draw at the specified position with the specified format
    Rect getBounds(DrawPoint pos, const std::string& text, FontStyle format) const;

    /// Gibt Infos, über die Unterbrechungspunkte in einem Text
    class WrapInfo
    {
    public:
        /// Erzeugt ein Arrays aus eigenständigen Strings aus den Unterbrechungsinfos.
        std::vector<std::string> CreateSingleStrings(const std::string& text);

        /// Array von Positionen, wo der Text umbrochen werden soll (jeweils der Anfang vom String)
        std::vector<unsigned> positions;
    };

    /// Gibt Infos, über die Unterbrechungspunkte in einem Text, versucht Wörter nicht zu trennen, tut dies aber, falls
    /// es unumgänglich ist (Wort länger als die Zeile)
    WrapInfo GetWrapInfo(const std::string& text, unsigned short primary_width, unsigned short secondary_width);

    struct CharInfo
    {
        CharInfo() : pos(0, 0), width(0) {}
        CharInfo(const Position& pos, unsigned width) : pos(pos), width(width) {}
        Position pos;
        unsigned width;
    };

    /// prüft ob ein Buchstabe existiert.
    bool CharExist(unsigned c) const;
    /// liefert die Breite eines Zeichens
    unsigned CharWidth(unsigned c) const { return GetCharInfo(c).width; }

private:
    using GlPoint = Point<GLfloat>;
    struct VertexArrays
    {
        std::vector<GlPoint> texCoords;
        std::vector<GlPoint> vertices;
    };

    void initFont();
    void ClearCharInfoMapping();
    void AddCharInfo(unsigned c, const CharInfo& info);
    /// liefert das Char-Info eines Zeichens
    const CharInfo& GetCharInfo(unsigned c) const;
    void DrawChar(unsigned curChar, VertexArrays& vertices, DrawPoint& curPos) const;

    std::unique_ptr<glArchivItem_Bitmap> fontNoOutline;
    std::unique_ptr<glArchivItem_Bitmap> fontWithOutline;

    /// Holds ascii chars only. As most chars are ascii this is faster then accessing the map
    std::array<std::pair<bool, CharInfo>, 256> asciiMapping;
    std::map<unsigned, CharInfo> utf8_mapping;
    CharInfo placeHolder; /// Placeholder if glyph is missing
    VertexArrays texList; /// Buffer to hold last textures. Used so memory reallocations are avoided

    /// Get width of the sequence defined by the begin/end pair of iterators
    template<class T_Iterator>
    unsigned getWidthInternal(const T_Iterator& begin, const T_Iterator& end) const;
    /// Same as above but the width will be at most maxWidth. The number of chars (or the iterator distance) is returned in maxNumChars
    template<class T_Iterator>
    unsigned getWidthInternal(const T_Iterator& begin, const T_Iterator& end, unsigned maxWidth, unsigned* maxNumChars) const;
    template<bool T_unlimitedWidth, class T_Iterator>
    unsigned getWidthInternal(const T_Iterator& begin, const T_Iterator& end, unsigned maxWidth, unsigned* maxNumChars) const;
};

#endif // !GLARCHIVITEM_FONT_H_INCLUDED
