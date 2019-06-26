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

#include "rttrDefines.h" // IWYU pragma: keep
#include "glArchivItem_Font.h"
#include "FontStyle.h"
#include "Loader.h"
#include "drivers/VideoDriverWrapper.h"
#include "glArchivItem_Bitmap.h"
#include "helpers/containerUtils.h"
#include "libsiedler2/ArchivItem_Bitmap_Player.h"
#include "libsiedler2/IAllocator.h"
#include "libsiedler2/PixelBufferARGB.h"
#include "libsiedler2/libsiedler2.h"
#include "libutil/Log.h"
#include <utf8.h>
#include <boost/algorithm/string.hpp>
#include <cmath>
#include <vector>

//#define RTTR_PRINT_FONTS

using utf8Iterator = utf8::iterator<std::string::const_iterator>;

template<typename T>
struct GetNextCharAndIncIt;

template<>
struct GetNextCharAndIncIt<char32_t>
{
    template<class T_Iterator>
    char32_t operator()(T_Iterator& it, const T_Iterator& /*itEnd*/) const
    {
        return *it++;
    }
};

template<>
struct GetNextCharAndIncIt<char>
{
    template<class T_Iterator>
    uint32_t operator()(T_Iterator& it, const T_Iterator& itEnd) const
    {
        return utf8::next(it, itEnd);
    }
};

template<typename T_Iterator, typename T_Value = typename std::iterator_traits<T_Iterator>::value_type>
struct MakePrevIt;

template<typename T_Iterator>
struct MakePrevIt<T_Iterator, char32_t>
{
    void operator()(T_Iterator& it, const T_Iterator& /*itStart*/) const { --it; }
};

template<typename T_Iterator>
struct MakePrevIt<T_Iterator, char>
{
    void operator()(T_Iterator& it, const T_Iterator& itStart) const { utf8::prior(it, itStart); }
};

template<class T_Iterator>
struct Distance
{
    size_t operator()(const T_Iterator& first, const T_Iterator& last) const { return std::distance(first, last); }
};

template<class T_Iterator>
struct Distance<utf8::iterator<T_Iterator>>
{
    size_t operator()(const utf8::iterator<T_Iterator>& first, const utf8::iterator<T_Iterator>& last) const
    {
        return std::distance(first.base(), last.base());
    }
};

template<class T_Iterator>
T_Iterator nextIt(T_Iterator it)
{
    return ++it;
}

//////////////////////////////////////////////////////////////////////////

glArchivItem_Font::glArchivItem_Font() : fontNoOutline(nullptr), fontWithOutline(nullptr)
{
    ClearCharInfoMapping();
}

glArchivItem_Font::glArchivItem_Font(const glArchivItem_Font& obj)
    : ArchivItem_Font(obj), asciiMapping(obj.asciiMapping), utf8_mapping(obj.utf8_mapping)
{
    fontNoOutline = libsiedler2::clone(obj.fontNoOutline);
    fontWithOutline = libsiedler2::clone(obj.fontWithOutline);
}

bool glArchivItem_Font::CharExist(unsigned c) const
{
    if(c < asciiMapping.size())
        return asciiMapping[c].first;
    return helpers::contains(utf8_mapping, c);
}

inline const glArchivItem_Font::CharInfo& glArchivItem_Font::GetCharInfo(unsigned c) const
{
    if(c < asciiMapping.size())
    {
        if(asciiMapping[c].first)
            return asciiMapping[c].second;
    } else
    {
        auto it = utf8_mapping.find(c);
        if(it != utf8_mapping.end())
            return it->second;
    }
    return placeHolder;
}

void glArchivItem_Font::ClearCharInfoMapping()
{
    using CharPair = std::pair<bool, CharInfo>;
    for(CharPair& entry : asciiMapping)
        entry.first = false;
    utf8_mapping.clear();
}

void glArchivItem_Font::AddCharInfo(unsigned c, const CharInfo& info)
{
    if(c < asciiMapping.size())
        asciiMapping[c] = std::make_pair(true, info);
    else
        utf8_mapping[c] = info;
}

/**
 *  @brief fügt ein einzelnes Zeichen zur Zeichenliste hinzu
 */
inline void glArchivItem_Font::DrawChar(unsigned curChar, VertexArrays& vertices, DrawPoint& curPos) const
{
    CharInfo ci = GetCharInfo(curChar);

    GlPoint texCoord1(ci.pos);
    GlPoint texCoord2(ci.pos + DrawPoint(ci.width, dy));

    vertices.texCoords.push_back(texCoord1);
    vertices.texCoords.push_back(GlPoint(texCoord1.x, texCoord2.y));
    vertices.texCoords.push_back(texCoord2);
    vertices.texCoords.push_back(GlPoint(texCoord2.x, texCoord1.y));

    GlPoint curPos1(curPos);
    GlPoint curPos2(curPos + DrawPoint(ci.width, dy));

    vertices.vertices.push_back(curPos1);
    vertices.vertices.push_back(GlPoint(curPos1.x, curPos2.y));
    vertices.vertices.push_back(curPos2);
    vertices.vertices.push_back(GlPoint(curPos2.x, curPos1.y));

    curPos.x += ci.width;
}

void glArchivItem_Font::Draw(DrawPoint pos, const std::u32string& wtext, FontStyle format, unsigned color, unsigned short length,
                             unsigned short maxWidth, const std::u32string& end)
{
    // etwas dämlich, aber einfach ;)
    // da wir hier erstmal in utf8 konvertieren, und dann im anderen Draw wieder zurück ...
    std::string text = utf8::utf32to8(wtext);
    std::string utf8End = utf8::utf32to8(end);
    Draw(pos, text, format, color, length, maxWidth, utf8End);
}

/**
 *  Zeichnet einen Text.
 *
 *  @param[in] x      X-Koordinate
 *  @param[in] y      Y-Koordinate
 *  @param[in] text   Der Text
 *  @param[in] format Format des Textes (verodern)
 *                      @p FontStyle::LEFT    - Text links ( standard )
 *                      @p FontStyle::CENTER  - Text mittig
 *                      @p FontStyle::RIGHT   - Text rechts
 *                      @p FontStyle::TOP     - Text oben ( standard )
 *                      @p FontStyle::VCENTER - Text vertikal zentriert
 *                      @p FontStyle::BOTTOM  - Text unten
 *  @param[in] color  Farbe des Textes
 *  @param[in] length Länge des Textes
 *  @param[in] max    maximale Länge
 *  @param     end    Suffix for displaying a truncation of the text (...)
 */
void glArchivItem_Font::Draw(DrawPoint pos, const std::string& text, FontStyle format, unsigned color, unsigned short length,
                             unsigned short maxWidth, const std::string& end)
{
    if(!fontNoOutline)
        initFont();

    RTTR_Assert(utf8::is_valid(text));

    // Breite bestimmen
    if(length == 0)
        length = (unsigned short)text.length();

    unsigned maxNumChars;
    unsigned short textWidth;
    bool drawEnd;
    if(maxWidth == 0xFFFF)
    {
        maxNumChars = length;
        textWidth = getWidth(text, length);
        drawEnd = false;
    } else
    {
        RTTR_Assert(utf8::is_valid(end));
        textWidth = getWidth(text, length, maxWidth, &maxNumChars);
        if(!end.empty() && maxNumChars < length)
        {
            unsigned short endWidth = getWidth(end);

            // If "end" does not fit, draw nothing
            if(textWidth < endWidth)
                return;

            // Wieviele Buchstaben gehen in den "Rest" (ohne "end")
            textWidth = getWidth(text, length, textWidth - endWidth, &maxNumChars) + endWidth;
            drawEnd = true;
        } else
            drawEnd = false;
    }

    if(maxNumChars == 0)
        return;
    auto itEnd = text.cbegin();
    std::advance(itEnd, maxNumChars);

    // Vertical alignment (assumes 1 line only!)
    if(format.is(FontStyle::BOTTOM))
        pos.y -= dy;
    else if(format.is(FontStyle::VCENTER))
        pos.y -= dy / 2;
    // Horizontal center must change current line only. Everything else changes the start point
    DrawPoint curPos(pos);
    // Horizontal alignment
    if(format.is(FontStyle::RIGHT))
        curPos.x = pos.x -= textWidth;
    else if(format.is(FontStyle::CENTER))
    {
        unsigned short line_width;
        const auto itNl = std::find(text.cbegin(), itEnd, '\n');
        if(itNl != itEnd)
            line_width = getWidthInternal(text.cbegin(), itNl);
        else
            line_width = textWidth;
        curPos.x = pos.x - line_width / 2;
    }

    texList.texCoords.clear();
    texList.vertices.clear();

    for(auto it = text.begin(); it != itEnd;)
    {
        const uint32_t curChar = utf8::next(it, itEnd);
        if(curChar == '\n')
        {
            if(format.is(FontStyle::CENTER))
            {
                unsigned short line_width;
                const auto itNl = std::find(it, itEnd, '\n');
                line_width = getWidthInternal(it, itNl);
                curPos.x = pos.x - line_width / 2;
            } else
                curPos.x = pos.x;
            curPos.y += dy;
        } else
            DrawChar(curChar, texList, curPos);
    }

    if(drawEnd)
    {
        for(auto it = end.begin(); it != end.end();)
        {
            const uint32_t curChar = utf8::next(it, end.end());
            if(curChar == '\n')
            {
                curPos.x = pos.x;
                curPos.y += dy;
            } else
                DrawChar(curChar, texList, curPos);
        }
    }

    if(texList.vertices.empty())
        return;

    // Get texture first as it might need to be created
    glArchivItem_Bitmap& usedFont = format.is(FontStyle::NO_OUTLINE) ? *fontNoOutline : *fontWithOutline;
    unsigned texture = usedFont.GetTexture();
    if(!texture)
        return;
    const GlPoint texSize(usedFont.GetTexSize());
    RTTR_Assert(texList.texCoords.size() == texList.vertices.size());
    RTTR_Assert(texList.texCoords.size() % 4u == 0);
    // Vectorizable loop
    for(unsigned i = 0; i < texList.texCoords.size(); i += 4)
    {
        for(int j = 0; j < 4; j++)
            texList.texCoords[i + j] /= texSize;
    }

    glVertexPointer(2, GL_FLOAT, 0, &texList.vertices[0]);
    glTexCoordPointer(2, GL_FLOAT, 0, &texList.texCoords[0]);
    VIDEODRIVER.BindTexture(texture);
    glColor4ub(GetRed(color), GetGreen(color), GetBlue(color), GetAlpha(color));
    glDrawArrays(GL_QUADS, 0, texList.vertices.size());
}

template<bool T_limitWidth, class T_Iterator>
unsigned glArchivItem_Font::getWidthInternal(const T_Iterator& begin, const T_Iterator& end, unsigned maxWidth, unsigned* maxNumChars) const
{
    unsigned curLineLen = 0, maxLineLen = 0;
    for(T_Iterator it = begin; it != end;)
    {
        const uint32_t curChar = GetNextCharAndIncIt<typename std::iterator_traits<T_Iterator>::value_type>()(it, end);
        if(curChar == '\n')
        {
            if(curLineLen > maxLineLen)
                maxLineLen = curLineLen;
            curLineLen = 0;
        } else
        {
            const unsigned cw = CharWidth(curChar);
            // If we limit the width and the current line will be longer, stop before it
            // Do not stop if this is the first char in this line
            if(T_limitWidth && curLineLen != 0 && curLineLen + cw > maxWidth)
            {
                MakePrevIt<T_Iterator>()(it, begin);
                *maxNumChars = static_cast<unsigned>(std::distance(begin, it));
                return curLineLen;
            }
            curLineLen += cw;
        }
    }

    if(T_limitWidth)
        *maxNumChars = static_cast<unsigned>(std::distance(begin, end));

    // Last line can be longest
    return std::max(curLineLen, maxLineLen);
}

template<class T_Iterator>
unsigned glArchivItem_Font::getWidthInternal(const T_Iterator& begin, const T_Iterator& end, unsigned maxWidth, unsigned* maxNumChars) const
{
    return getWidthInternal<true>(begin, end, maxWidth, maxNumChars);
}

template<class T_Iterator>
unsigned glArchivItem_Font::getWidthInternal(const T_Iterator& begin, const T_Iterator& end) const
{
    return getWidthInternal<false>(begin, end, 0, nullptr);
}

unsigned short glArchivItem_Font::getWidth(const std::u32string& text, unsigned length, unsigned maxWidth, unsigned* maxNumChars) const
{
    return getWidthInternal(text.begin(), length ? text.begin() + length : text.end(), maxWidth, maxNumChars);
}

unsigned short glArchivItem_Font::getWidth(const std::string& text, unsigned length, unsigned maxWidth, unsigned* maxNumChars) const
{
    return getWidthInternal(text.begin(), length ? text.begin() + length : text.end(), maxWidth, maxNumChars);
}

unsigned short glArchivItem_Font::getWidth(const std::u32string& text, unsigned length) const
{
    return getWidthInternal(text.begin(), length ? text.begin() + length : text.end());
}

unsigned short glArchivItem_Font::getWidth(const std::string& text, unsigned length) const
{
    return getWidthInternal(text.begin(), length ? text.begin() + length : text.end());
}

Rect glArchivItem_Font::getBounds(DrawPoint pos, const std::string& text, FontStyle format) const
{
    if(text.empty())
        return Rect(Position(pos), 0, 0);
    unsigned width = getWidth(text);
    unsigned numLines = static_cast<unsigned>(std::count(text.begin(), text.end(), '\n')) + 1;
    Rect result(Position(pos), width, numLines * getHeight());
    Position offset(0, 0);
    if(format.is(FontStyle::RIGHT))
        offset.x = width;
    else if(format.is(FontStyle::CENTER))
        offset.x = width / 2;
    if(format.is(FontStyle::BOTTOM))
        offset.y = getHeight();
    else if(format.is(FontStyle::VCENTER))
        offset.y = getHeight() / 2;
    result.move(-offset);
    return result;
}

/**
 *  @brief
 */
std::vector<std::string> glArchivItem_Font::WrapInfo::CreateSingleStrings(const std::string& text)
{
    RTTR_Assert(utf8::is_valid(text)); // Can only handle UTF-8 strings!

    std::vector<std::string> destStrings;
    if(positions.empty())
        return destStrings;

    destStrings.reserve(positions.size());
    unsigned curStart = positions.front();
    for(auto it = positions.begin() + 1; it != positions.end(); ++it)
    {
        RTTR_Assert(*it >= curStart);
        std::string curLine = text.substr(curStart, *it - curStart);
        boost::algorithm::trim_right(curLine);
        destStrings.push_back(curLine);
        curStart = *it;
    }
    /* Push last part */
    destStrings.push_back(text.substr(curStart));
    return destStrings;
}

/**
 *  Gibt Infos, über die Unterbrechungspunkte in einem Text
 *
 *  @param[in]     text            Text, der auf Zeilen verteilt werden soll
 *  @param[in]     primary_width   Maximale Breite der ersten Zeile
 *  @param[in]     secondary_width Maximale Breite der weiteren Zeilen
 */
glArchivItem_Font::WrapInfo glArchivItem_Font::GetWrapInfo(const std::string& text, const unsigned short primary_width,
                                                           const unsigned short secondary_width)
{
    if(!fontNoOutline)
        initFont();

    RTTR_Assert(utf8::is_valid(text)); // Can only handle UTF-8 strings!

    // Current line width
    unsigned line_width = 0;
    // Width of current word
    unsigned word_width = 0;
    unsigned curMaxLineWidth = primary_width;

    WrapInfo wi;
    // We start the first line at the first char (so wi.positions.size() == numLines)
    wi.positions.push_back(0);

    utf8Iterator it(text.begin(), text.begin(), text.end());
    utf8Iterator itEnd(text.end(), text.begin(), text.end());
    utf8Iterator itWordStart = it;

    const unsigned spaceWidth = CharWidth(' ');

    for(;; ++it)
    {
        uint32_t curChar = (it != itEnd) ? *it : 0;
        // Word ended
        if(curChar == 0 || curChar == '\n' || curChar == ' ')
        {
            // Is the current word to long for the current line
            if(word_width + line_width > curMaxLineWidth)
            {
                // Word does not fit -> Start new line

                // Can we fit the word in one line?
                if(word_width <= secondary_width)
                {
                    // New line starts at index of word start
                    wi.positions.push_back(static_cast<unsigned>(itWordStart.base() - text.begin()));
                    line_width = 0;
                } else
                {
                    // Word does not even fit on one line -> Put as many letters in one line as possible
                    for(utf8Iterator itWord = itWordStart; itWord != it; ++itWord)
                    {
                        unsigned letter_width = CharWidth(*itWord);

                        // Can we fit the letter onto current line?
                        if(line_width + letter_width <= curMaxLineWidth)
                            line_width += letter_width; // Add it
                        else
                        {
                            // Create new line at this letter
                            wi.positions.push_back(static_cast<unsigned>(itWord.base() - text.begin()));
                            line_width = letter_width;
                            itWordStart = nextIt(itWord);
                        }
                    }

                    // Restart word
                    word_width = 0;
                }
                curMaxLineWidth = secondary_width;
            }
            if(curChar == 0)
                break;
            else if(curChar == ' ')
            {
                // Set up this line if we are going to continue it (not at line break or text end)
                // Line contains word and whitespace
                line_width += word_width + spaceWidth;
                word_width = 0;
                itWordStart = nextIt(it);
            } else
            {
                // If line break add new line (after all the word-breaking above)
                itWordStart = nextIt(it);
                if(itWordStart == itEnd)
                    break; // Reached end
                word_width = 0;
                line_width = 0;
                wi.positions.push_back(static_cast<unsigned>(itWordStart.base() - text.begin()));
            }
        } else
        {
            // Some char -> Add its width
            word_width += CharWidth(curChar);
        }
    }

    // Ignore trailing newline
    if(wi.positions.back() + 1 >= text.length() && wi.positions.size() > 1)
        wi.positions.pop_back();
    return wi;
}

/**
 *  @brief
 */
void glArchivItem_Font::initFont()
{
    ClearCharInfoMapping();
    fontWithOutline = libsiedler2::getAllocator().create<glArchivItem_Bitmap>(libsiedler2::BOBTYPE_BITMAP_RLE);
    fontNoOutline = libsiedler2::getAllocator().create<glArchivItem_Bitmap>(libsiedler2::BOBTYPE_BITMAP_RLE);

    // first, we have to find how much chars we really have
    unsigned numChars = 0;
    for(unsigned i = 0; i < size(); ++i)
    {
        if(get(i))
            ++numChars;
    }

    if(numChars == 0)
        return;

    const auto numCharsPerLine = static_cast<unsigned>(std::sqrt(static_cast<double>(numChars)));
    // Calc lines required (rounding up)
    const unsigned numLines = (numChars + numCharsPerLine - 1) / numCharsPerLine;

    constexpr Extent spacing(1, 1);
    Extent texSize = (Extent(dx, dy) + spacing * 2u) * Extent(numCharsPerLine, numLines) + spacing * 2u;
    libsiedler2::PixelBufferARGB bufferWithOutline(texSize.x, texSize.y);
    libsiedler2::PixelBufferARGB bufferNoOutline(texSize.x, texSize.y);

    const libsiedler2::ArchivItem_Palette* const palette = LOADER.GetPaletteN("colors");
    Position curPos(spacing);
    numChars = 0;
    for(unsigned i = 0; i < size(); ++i)
    {
        const auto* c = dynamic_cast<const libsiedler2::ArchivItem_Bitmap_Player*>(get(i));
        if(!c)
            continue;

        if((numChars % numCharsPerLine) == 0 && numChars > 0)
        {
            curPos.y += dy + spacing.y * 2;
            curPos.x = spacing.x;
        }

        // Spezialpalette (blaue Spielerfarben sind Grau) verwenden, damit man per OpenGL einfärben kann!
        c->print(bufferNoOutline, palette, 128, curPos.x, curPos.y, 0, 0, 0, 0, true);
        c->print(bufferWithOutline, palette, 128, curPos.x, curPos.y);

        CharInfo ci(curPos, std::min<unsigned short>(dx + 2, c->getWidth()));

        AddCharInfo(i, ci);
        curPos.x += dx + spacing.x * 2;
        ++numChars;
    }

    fontNoOutline->create(bufferNoOutline);
    fontWithOutline->create(bufferWithOutline);

    // Set the placeholder for non-existant glyphs. Use '?' (should always be possible)
    if(CharExist('?'))
        placeHolder = GetCharInfo('?');
    else
    {
        // Fall back to first glyph in map (kinda random, but should not happen anyway)
        LOG.writeToFile("Cannot find '?' glyph in font!");
        throw std::runtime_error("Cannot find '?' glyph in font. What shall I use as fallback?");
    }

#ifdef RTTR_PRINT_FONTS
    Archiv items;
    items.pushC(*fontNoOutline);
    libsiedler2::Write("font" + getName() + "_noOutline.bmp", items);
    items.setC(0, *fontWithOutline);
    libsiedler2::Write("font" + getName() + "_Outline.bmp", items);
#endif
}
