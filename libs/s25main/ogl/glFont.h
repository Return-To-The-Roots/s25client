// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "DrawPoint.h"
#include "Rect.h"
#include "ogl/FontStyle.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "s25util/colors.h"
#ifdef __EMSCRIPTEN__
#include "SDL/SDL.h"
#include "SDL/SDL_opengl.h"
#else
#include <glad/glad.h>
#endif
#include <array>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace libsiedler2 {
class ArchivItem_Font;
}

class glFont
{
public:
    glFont(const libsiedler2::ArchivItem_Font&);

    /// Draw the the text at the given position with format (alignment) and color.
    /// If maxWidth is given then the text length will be at most maxWidth. If the text is shortened then end is
    /// appended (included in maxWidth)
    void Draw(DrawPoint pos, const std::string& text, FontStyle format, unsigned color = COLOR_WHITE,
              unsigned short maxWidth = 0xFFFF, const std::string& end = "...") const;

    /// Return the width of the drawn text. If maxWidth is given then the width will be <= maxWidth and maxNumChars will
    /// be set to the maximum number of chars (not glyphs!) that fit into the width
    unsigned getWidth(const std::string& text) const;
    unsigned getWidth(const std::string& text, unsigned maxWidth, unsigned* maxNumChars) const;
    /// Return height of each line of text
    unsigned short getHeight() const { return maxCharSize.y + 1; }
    unsigned getDx() const { return maxCharSize.x; }

    /// Return the bounds of the text when draw at the specified position with the specified format
    Rect getBounds(DrawPoint pos, const std::string& text, FontStyle format) const;

    /// Gibt Infos, über die Unterbrechungspunkte in einem Text
    class WrapInfo
    {
    public:
        struct LineRange
        {
            unsigned start, len;
        };

        /// Create an array of strings by breaking the given string at the contained line range information
        std::vector<std::string> CreateSingleStrings(const std::string& text) const;

        /// Information on each line detected
        std::vector<LineRange> lines;
    };

    /// Gibt Infos, über die Unterbrechungspunkte in einem Text, versucht Wörter nicht zu trennen, tut dies aber, falls
    /// es unumgänglich ist (Wort länger als die Zeile)
    WrapInfo GetWrapInfo(const std::string& text, unsigned short primary_width, unsigned short secondary_width) const;

    /// prüft ob ein Buchstabe existiert.
    bool CharExist(char32_t c) const;
    /// liefert die Breite eines Zeichens
    unsigned CharWidth(char32_t c) const { return GetCharInfo(c).width; }

private:
    struct CharInfo
    {
        CharInfo() : pos(0, 0), width(0) {}
        CharInfo(const Position& pos, unsigned width) : pos(pos), width(width) {}
        Position pos;
        unsigned width;
    };
    using GlPoint = Point<GLfloat>;
    struct VertexArrays
    {
        std::vector<GlPoint> texCoords;
        std::vector<GlPoint> vertices;
    };

    void AddCharInfo(char32_t c, const CharInfo& info);
    /// liefert das Char-Info eines Zeichens
    const CharInfo& GetCharInfo(char32_t c) const;
    void DrawChar(char32_t curChar, VertexArrays& vertices, DrawPoint& curPos) const;

    Extent maxCharSize; // How big each char is at most (aka dx,dy)
    std::unique_ptr<glArchivItem_Bitmap> fontNoOutline;
    std::unique_ptr<glArchivItem_Bitmap> fontWithOutline;

    /// Holds ascii chars only. As most chars are ascii this is faster then accessing the map
    std::array<std::pair<bool, CharInfo>, 256> asciiMapping;
    std::map<char32_t, CharInfo> utf8_mapping;
    CharInfo placeHolder;         /// Placeholder if glyph is missing
    mutable VertexArrays texList; /// Buffer to hold last textures. Used so memory reallocations are avoided

    /// Get width of the sequence defined by the begin/end pair of iterators
    template<bool T_unlimitedWidth>
    unsigned getWidthInternal(const std::string::const_iterator& begin, const std::string::const_iterator& end,
                              unsigned maxWidth, unsigned* maxNumChars) const;
};
