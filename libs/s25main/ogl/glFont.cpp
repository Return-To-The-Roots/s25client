// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "glFont.h"
#include "FontStyle.h"
#include "Loader.h"
#include "drivers/VideoDriverWrapper.h"
#include "glArchivItem_Bitmap.h"
#include "helpers/containerUtils.h"
#include "libsiedler2/ArchivItem_Bitmap_Player.h"
#include "libsiedler2/ArchivItem_Font.h"
#include "libsiedler2/IAllocator.h"
#include "libsiedler2/PixelBufferBGRA.h"
#include "libsiedler2/libsiedler2.h"
#include "s25util/utf8.h"
#include <boost/algorithm/string.hpp>
#include <boost/nowide/detail/utf.hpp>
#include <cmath>
#include <vector>

constexpr bool RTTR_PRINT_FONTS = false;
namespace utf = boost::nowide::detail::utf;
using utf8 = utf::utf_traits<char>;

//////////////////////////////////////////////////////////////////////////

glFont::glFont(const libsiedler2::ArchivItem_Font& font) : maxCharSize(font.getDx(), font.getDy()), asciiMapping{}
{
    fontWithOutline = libsiedler2::getAllocator().create<glArchivItem_Bitmap>(libsiedler2::BobType::Bitmap);
    fontNoOutline = libsiedler2::getAllocator().create<glArchivItem_Bitmap>(libsiedler2::BobType::Bitmap);

    // first, we have to find how much chars we really have
    unsigned numChars = 0;
    for(unsigned i = 0; i < font.size(); ++i)
    {
        if(font[i])
            ++numChars;
    }

    if(numChars == 0)
        return;

    const auto numCharsPerLine = static_cast<unsigned>(std::sqrt(static_cast<double>(numChars)));
    // Calc lines required (rounding up)
    const unsigned numLines = (numChars + numCharsPerLine - 1) / numCharsPerLine;

    constexpr Extent spacing(1, 1);
    Extent texSize = (maxCharSize + spacing * 2u) * Extent(numCharsPerLine, numLines) + spacing * 2u;
    libsiedler2::PixelBufferBGRA bufferWithOutline(texSize.x, texSize.y);
    libsiedler2::PixelBufferBGRA bufferNoOutline(texSize.x, texSize.y);

    const libsiedler2::ArchivItem_Palette* const palette = LOADER.GetPaletteN("colors");
    Position curPos(spacing);
    numChars = 0;
    for(unsigned i = 0; i < font.size(); ++i)
    {
        const auto* c = dynamic_cast<const libsiedler2::ArchivItem_Bitmap_Player*>(font[i]);
        if(!c)
            continue;

        if((numChars % numCharsPerLine) == 0 && numChars > 0)
        {
            curPos.y += maxCharSize.y + spacing.y * 2;
            curPos.x = spacing.x;
        }

        // Spezialpalette (blaue Spielerfarben sind Grau) verwenden, damit man per OpenGL einfärben kann!
        c->print(bufferNoOutline, palette, 128, curPos.x, curPos.y, 0, 0, 0, 0, true);
        c->print(bufferWithOutline, palette, 128, curPos.x, curPos.y);

        CharInfo ci(curPos, std::min<unsigned short>(maxCharSize.x + 2, c->getWidth()));

        AddCharInfo(i, ci);
        curPos.x += maxCharSize.x + spacing.x * 2;
        ++numChars;
    }

    fontNoOutline->create(bufferNoOutline);
    fontWithOutline->create(bufferWithOutline);

    // Set the placeholder for non-existant glyphs. Use '?' (should always be possible)
    if(CharExist(0xFFFD))
        placeHolder = GetCharInfo(0xFFFD);
    else if(CharExist('?'))
        placeHolder = GetCharInfo('?');
    else
        throw std::runtime_error("Cannot find '?' glyph in font. What shall I use as fallback?");

    if(RTTR_PRINT_FONTS)
    {
        libsiedler2::Archiv items;
        items.pushC(*fontNoOutline);
        libsiedler2::Write("font" + font.getName() + "_noOutline.bmp", items);
        items.setC(0, *fontWithOutline);
        libsiedler2::Write("font" + font.getName() + "_Outline.bmp", items);
    }
}

bool glFont::CharExist(char32_t c) const
{
    if(c < asciiMapping.size())
        return asciiMapping[c].first;
    return helpers::contains(utf8_mapping, c);
}

const glFont::CharInfo& glFont::GetCharInfo(char32_t c) const
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

void glFont::AddCharInfo(char32_t c, const CharInfo& info)
{
    if(c < asciiMapping.size())
        asciiMapping[c] = std::make_pair(true, info);
    else
        utf8_mapping[c] = info;
}

/**
 *  @brief fügt ein einzelnes Zeichen zur Zeichenliste hinzu
 */
inline void glFont::DrawChar(char32_t curChar, VertexArrays& vertices, DrawPoint& curPos) const
{
    CharInfo ci = GetCharInfo(curChar);

    GlPoint texCoord1(ci.pos);
    GlPoint texCoord2(ci.pos + DrawPoint(ci.width, maxCharSize.y));

    vertices.texCoords.push_back(texCoord1);
    vertices.texCoords.push_back(GlPoint(texCoord1.x, texCoord2.y));
    vertices.texCoords.push_back(texCoord2);
    vertices.texCoords.push_back(GlPoint(texCoord2.x, texCoord1.y));

    GlPoint curPos1(curPos);
    GlPoint curPos2(curPos + DrawPoint(ci.width, maxCharSize.y));

    vertices.vertices.push_back(curPos1);
    vertices.vertices.push_back(GlPoint(curPos1.x, curPos2.y));
    vertices.vertices.push_back(curPos2);
    vertices.vertices.push_back(GlPoint(curPos2.x, curPos1.y));

    curPos.x += ci.width;
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
void glFont::Draw(DrawPoint pos, const std::string& text, FontStyle format, unsigned color, unsigned short maxWidth,
                  const std::string& end) const
{
    RTTR_Assert(s25util::isValidUTF8(text));

    unsigned maxNumChars;
    unsigned short textWidth;
    bool drawEnd;
    if(maxWidth == 0xFFFF)
    {
        maxNumChars = text.size();
        textWidth = getWidth(text);
        drawEnd = false;
    } else
    {
        RTTR_Assert(s25util::isValidUTF8(end));
        textWidth = getWidth(text, maxWidth, &maxNumChars);
        if(!end.empty() && maxNumChars < text.size())
        {
            unsigned short endWidth = getWidth(end);

            // If "end" does not fit, draw nothing
            if(textWidth < endWidth)
                return;

            // Wieviele Buchstaben gehen in den "Rest" (ohne "end")
            textWidth = getWidth(text, textWidth - endWidth, &maxNumChars) + endWidth;
            drawEnd = true;
        } else
            drawEnd = false;
    }

    if(maxNumChars == 0)
        return;
    const auto itEnd = text.cbegin() + maxNumChars;

    // Vertical alignment (assumes 1 line only!)
    if(format.is(FontStyle::BOTTOM))
        pos.y -= maxCharSize.y;
    else if(format.is(FontStyle::VCENTER))
        pos.y -= maxCharSize.y / 2;
    // Horizontal alignment
    if(format.is(FontStyle::RIGHT))
        pos.x -= textWidth;
    else if(format.is(FontStyle::CENTER))
        pos.x -= textWidth / 2;

    texList.texCoords.clear();
    texList.vertices.clear();

    for(auto it = text.begin(); it != itEnd;)
    {
        const utf::code_point curChar = utf8::decode(it, itEnd);
        DrawChar(curChar, texList, pos);
    }

    if(drawEnd)
    {
        for(auto it = end.begin(); it != end.end();)
        {
            const utf::code_point curChar = utf8::decode(it, end.end());
            DrawChar(curChar, texList, pos);
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
    for(GlPoint& pt : texList.texCoords)
        pt /= texSize;

    glVertexPointer(2, GL_FLOAT, 0, &texList.vertices[0]);
    glTexCoordPointer(2, GL_FLOAT, 0, &texList.texCoords[0]);
    VIDEODRIVER.BindTexture(texture);
    glColor4ub(GetRed(color), GetGreen(color), GetBlue(color), GetAlpha(color));
    glDrawArrays(GL_QUADS, 0, texList.vertices.size());
}

template<bool T_limitWidth>
unsigned glFont::getWidthInternal(const std::string::const_iterator& begin, const std::string::const_iterator& end,
                                  unsigned maxWidth, unsigned* maxNumChars) const
{
    unsigned curLen = 0;
    for(auto it = begin; it != end;)
    {
        const auto itCurChar = it;
        const utf::code_point curChar = utf8::decode(it, end);
        const unsigned cw = CharWidth(curChar);
        // If we limit the width and the text will be longer, stop before it
        // Do not stop if this is the first char
        if(T_limitWidth && curLen != 0 && curLen + cw > maxWidth)
        {
            *maxNumChars = static_cast<unsigned>(std::distance(begin, itCurChar));
            return curLen;
        }
        curLen += cw;
    }

    if(T_limitWidth)
        *maxNumChars = static_cast<unsigned>(std::distance(begin, end));
    return curLen;
}

unsigned glFont::getWidth(const std::string& text) const
{
    return getWidthInternal<false>(text.begin(), text.end(), 0, nullptr);
}

unsigned glFont::getWidth(const std::string& text, unsigned maxWidth, unsigned* maxNumChars) const
{
    return getWidthInternal<true>(text.begin(), text.end(), maxWidth, maxNumChars);
}

Rect glFont::getBounds(DrawPoint pos, const std::string& text, FontStyle format) const
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

std::vector<std::string> glFont::WrapInfo::CreateSingleStrings(const std::string& text) const
{
    std::vector<std::string> destStrings;
    destStrings.reserve(lines.size());
    for(const auto& line : lines)
        destStrings.emplace_back(text.substr(line.start, line.len));
    return destStrings;
}

/**
 *  Gibt Infos, über die Unterbrechungspunkte in einem Text
 *
 *  @param[in]     text            Text, der auf Zeilen verteilt werden soll
 *  @param[in]     primary_width   Maximale Breite der ersten Zeile
 *  @param[in]     secondary_width Maximale Breite der weiteren Zeilen
 */
glFont::WrapInfo glFont::GetWrapInfo(const std::string& text, const unsigned short primary_width,
                                     const unsigned short secondary_width) const
{
    RTTR_Assert(s25util::isValidUTF8(text)); // Can only handle UTF-8 strings!

    // Current line width
    unsigned line_width = 0;
    // Width of current word
    unsigned word_width = 0;
    unsigned curMaxLineWidth = primary_width;

    WrapInfo wi;

    auto it = text.begin();
    const auto itEnd = text.end();
    auto itWordStart = it;
    auto itLineStart = it;

    const unsigned spaceWidth = CharWidth(' ');

    const auto makeLineRange = [&text, &itLineStart](const auto& itLineEnd) {
        return WrapInfo::LineRange{static_cast<unsigned>(itLineStart - text.begin()),
                                   static_cast<unsigned>(itLineEnd - itLineStart)};
    };

    while(true)
    {
        // Save iterator to current char as we might want to break BEFORE it
        const auto itCurChar = it;
        const utf::code_point curChar = (it != itEnd) ? utf8::decode(it, itEnd) : 0;
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
                    // Break before word
                    wi.lines.emplace_back(makeLineRange(itWordStart));
                    // New line starts at index of word start
                    itLineStart = itWordStart;
                    line_width = 0;
                } else
                {
                    // Word does not even fit on one line -> Put as many letters in one line as possible
                    for(auto itWord = itWordStart; itWord != itCurChar;)
                    {
                        const auto itPotentialBreak = itWord;
                        const utf::code_point letter_width = CharWidth(utf8::decode(itWord, itEnd));

                        // Can we fit the letter onto current line?
                        if(line_width + letter_width <= curMaxLineWidth)
                            line_width += letter_width; // Add it
                        else
                        {
                            // Create new line at this letter
                            wi.lines.emplace_back(makeLineRange(itPotentialBreak));
                            // New line starts at index of word start
                            itLineStart = itPotentialBreak;
                            line_width = letter_width;
                        }
                    }

                    // Restart word
                    word_width = 0;
                }
                curMaxLineWidth = secondary_width;
            }
            if(curChar == 0)
            {
                wi.lines.emplace_back(makeLineRange(itCurChar));
                break;
            } else if(curChar == ' ')
            {
                // Set up this line if we are going to continue it (not at line break or text end)
                // Line contains word and whitespace
                line_width += word_width + spaceWidth;
                word_width = 0;
                itWordStart = it;
            } else
            {
                // If line break add new line (after all the word-breaking above)
                wi.lines.emplace_back(makeLineRange(itCurChar));
                itLineStart = itWordStart = it;
                word_width = line_width = 0;
            }
        } else
        {
            // Some char -> Add its width
            word_width += CharWidth(curChar);
        }
    }
    for(auto& line : wi.lines)
    {
        while(line.len > 0 && text[line.start + line.len - 1] == ' ')
            --line.len;
    }
    return wi;
}
