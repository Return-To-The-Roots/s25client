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

#include "defines.h" // IWYU pragma: keep
#include "glArchivItem_Font.h"
#include "Settings.h"
#include "ExtensionList.h"
#include "drivers/VideoDriverWrapper.h"
#include "glArchivItem_Bitmap.h"
#include "Loader.h"
#include "Log.h"

#include "libsiedler2/src/ArchivItem_Bitmap_Player.h"
#include "libsiedler2/src/libsiedler2.h"
#include "libsiedler2/src/IAllocator.h"
#include "libutil/utf8/utf8.h"
#include <boost/algorithm/string.hpp>
#include <cmath>
#include <vector>

typedef utf8::iterator<std::string::const_iterator> utf8Iterator;


template<typename T>
struct GetNextCharAndIncIt;

template<>
struct GetNextCharAndIncIt<uint32_t>
{
    template<class T_Iterator>
    uint32_t operator()(T_Iterator& it, const T_Iterator& itEnd) const
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

template<class T_Iterator>
struct Distance
{
    size_t operator()(const T_Iterator& first, const T_Iterator& last) const
    {
        return std::distance(first, last);
    }
};

template<class T_Iterator>
struct Distance<utf8::iterator<T_Iterator> >
{
    size_t operator()(const utf8::iterator<T_Iterator>& first, const utf8::iterator<T_Iterator>& last) const
    {
        return std::distance(first.base(), last.base());
    }
};

template<class T_Iterator>
T_Iterator nextIt(T_Iterator it, typename std::iterator_traits<T_Iterator>::difference_type n = 1)
{
    std::advance(it, n);
    return it;
}

//////////////////////////////////////////////////////////////////////////

glArchivItem_Font::glArchivItem_Font(const glArchivItem_Font& obj): ArchivItem_Font(obj), utf8_mapping(obj.utf8_mapping)
{
    if(obj.fontNoOutline)
        fontNoOutline.reset(dynamic_cast<glArchivItem_Bitmap*>(libsiedler2::getAllocator().clone(*obj.fontNoOutline)));
    if(obj.fontWithOutline)
        fontWithOutline.reset(dynamic_cast<glArchivItem_Bitmap*>(libsiedler2::getAllocator().clone(*obj.fontWithOutline)));
}

glArchivItem_Font& glArchivItem_Font::operator=(const glArchivItem_Font& obj)
{
    if(this == &obj)
        return *this;

    libsiedler2::ArchivItem_Font::operator=(obj);
    utf8_mapping = obj.utf8_mapping;
    if(obj.fontNoOutline)
        fontNoOutline.reset(dynamic_cast<glArchivItem_Bitmap*>(libsiedler2::getAllocator().clone(*obj.fontNoOutline)));
    else
        fontNoOutline.reset();
    if(obj.fontWithOutline)
        fontWithOutline.reset(dynamic_cast<glArchivItem_Bitmap*>(libsiedler2::getAllocator().clone(*obj.fontWithOutline)));
    else
        fontWithOutline.reset();
    return *this;
}

inline const glArchivItem_Font::CharInfo& glArchivItem_Font::GetCharInfo(unsigned int c) const
{
    std::map<unsigned int, CharInfo>::const_iterator it = utf8_mapping.find(c);
    if(it != utf8_mapping.end())
        return it->second;

    return placeHolder;
}

/**
 *  @brief fügt ein einzelnes Zeichen zur Zeichenliste hinzu
 */
inline void glArchivItem_Font::DrawChar(const unsigned c,
                                        std::vector<GL_T2F_V3F_Struct>& vertices,
                                        short& cx,
                                        short& cy, //-V669
                                        float tw,
                                        float th) const
{
    CharInfo ci = GetCharInfo(c);

    float tx1 = static_cast<float>(ci.x) / tw;
    float tx2 = static_cast<float>(ci.x + ci.width) / tw;
    float ty1 = static_cast<float>(ci.y) / th;
    float ty2 = static_cast<float>(ci.y + dy) / th;

    GL_T2F_V3F_Struct tmp;
    tmp.tx = tx1;
    tmp.ty = ty1;
    tmp.x = cx;
    tmp.y = cy;
    tmp.z = 0.0f;
    vertices.push_back(tmp);

    tmp.tx = tx1;
    tmp.ty = ty2;
    tmp.x = cx;
    tmp.y = (GLfloat)(cy + dy);
    tmp.z = 0.0f;
    vertices.push_back(tmp);

    tmp.tx = tx2;
    tmp.ty = ty2;
    tmp.x = (GLfloat)(cx + ci.width);
    tmp.y = (GLfloat)(cy + dy);
    tmp.z = 0.0f;
    vertices.push_back(tmp);

    tmp.tx = tx2;
    tmp.ty = ty1;
    tmp.x = (GLfloat)(cx + ci.width);
    tmp.y = cy;
    tmp.z = 0.0f;
    vertices.push_back(tmp);

    cx += ci.width;
}

/**
 *  @brief
 */
void glArchivItem_Font::Draw(DrawPoint pos,
                             const ucString& wtext,
                             unsigned int format,
                             unsigned int color,
                             unsigned short length,
                             unsigned short max,
                             const ucString& wend)
{
    // etwas dämlich, aber einfach ;)
    // da wir hier erstmal in utf8 konvertieren, und dann im anderen Draw wieder zurück ...
    std::string text = cvUnicodeToUTF8(wtext);
    std::string end = cvUnicodeToUTF8(wend);
    Draw(pos, text, format, color, length, max, end);
}

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
 *  @param     end    Suffix for displaying a truncation of the text (...)
 */
void glArchivItem_Font::Draw(DrawPoint pos,
                             const std::string& textIn,
                             unsigned int format,
                             unsigned int color,
                             unsigned short length,
                             unsigned short max,
                             const std::string& end)
{
    if(!fontNoOutline)
        initFont();

    std::string text;
    if(!utf8::is_valid(textIn.begin(), textIn.end()))
    {
        RTTR_Assert(false); // Can only handle UTF-8 strings!
        // However old savegames/replays might contain invalid chars -> Replace
        // TODO: Remove this after next savegame version bump (cur: 31)!
        text.reserve(textIn.length());
        utf8::replace_invalid(textIn.begin(), textIn.end(), std::back_inserter(text));
    } else
        text = textIn;

    RTTR_Assert(utf8::is_valid(end.begin(), end.end()));

    // Breite bestimmen
    if(length == 0)
        length = (unsigned short)text.length();

    unsigned maxNumChars = 0;
    unsigned short textWidth = getWidth(text, length, max, &maxNumChars);

    bool drawEnd;
    if(!end.empty() && maxNumChars < length)
    {
        unsigned short endWidth = getWidth(end, 0, max);

        // If "end" does not fit, draw nothing
        if(textWidth < endWidth)
            return;

        // Wieviele Buchstaben gehen in den "Rest" (ohne "end")
        textWidth = getWidth(text, length, textWidth - endWidth, &maxNumChars) + endWidth;
        drawEnd = true;
    } else
        drawEnd = false;

    if(maxNumChars == 0)
        return;
    std::string::iterator itEnd = text.begin();
    std::advance(itEnd, maxNumChars);

    if( (format & 3) == DF_RIGHT)
        pos.x -= textWidth;
    if( (format & 12) == DF_BOTTOM)
        pos.y -= dy;
    else if( (format & 12) == DF_VCENTER)
        pos.y -= dy / 2;

    short cx = pos.x, cy = pos.y;
    if( (format & 3) == DF_CENTER)
    {
        unsigned short line_width;
        std::string::iterator itNl = std::find(text.begin(), itEnd, '\n');
        if(itNl != itEnd)
            line_width = getWidthInternal(text.begin(), itNl);
        else
            line_width = textWidth;
        cx = pos.x - line_width / 2;
    }

    std::vector<GL_T2F_V3F_Struct> texList;
    texList.reserve((maxNumChars + end.length()) * 4);
    float tw = fontNoOutline->GetTexWidth();
    float th = fontNoOutline->GetTexHeight();
    
    for(std::string::iterator it = text.begin(); it != itEnd;)
    {
        const uint32_t curChar = utf8::next(it, text.end());
        if(curChar == '\n')
        {
            cy += dy;
            if( (format & 3) == DF_CENTER)
            {
                unsigned short line_width;
                std::string::iterator itNext = nextIt(it);
                std::string::iterator itNl = std::find(itNext, itEnd, '\n');
                line_width = getWidthInternal(itNext, itNl);
                cx = pos.x - line_width / 2;
            }
            else
                cx = pos.x;
        }
        else
            DrawChar(curChar, texList, cx, cy, tw, th);
    }

    if(drawEnd)
    {
        for(std::string::const_iterator it = end.begin(); it != end.end();)
        {
            const uint32_t curChar = utf8::next(it, end.end());
            if(curChar == '\n')
            {
                cy += dy;
                cx = pos.x;
            } else
                DrawChar(curChar, texList, cx, cy, tw, th);
        }
    }

    if(texList.empty())
        return;

    glVertexPointer(3, GL_FLOAT, sizeof(GL_T2F_V3F_Struct), &texList[0].x);
    glTexCoordPointer(2, GL_FLOAT, sizeof(GL_T2F_V3F_Struct), &texList[0].tx);
    VIDEODRIVER.BindTexture(((format & DF_NO_OUTLINE) == DF_NO_OUTLINE) ? fontNoOutline->GetTexture() : fontWithOutline->GetTexture());
    glColor4ub(GetRed(color), GetGreen(color), GetBlue(color), GetAlpha(color));
    glDrawArrays(GL_QUADS, 0, texList.size());
}

template<class T_Iterator>
unsigned glArchivItem_Font::getWidthInternal(const T_Iterator& begin, const T_Iterator& end, unsigned maxWidth, unsigned* maxNumChars) const
{
    unsigned curLineLen = 0, maxLineLen = 0;
    for(T_Iterator it = begin; it != end;)
    {
        const uint32_t curChar = GetNextCharAndIncIt<typename std::iterator_traits<T_Iterator>::value_type>()(it, end);
        if(curChar == '\n')
        {
            if(curLineLen > maxLineLen) // Längste Zeile
                maxLineLen = curLineLen;
            curLineLen = 0;
        } else
        {
            const unsigned cw = CharWidth(curChar);
            // haben wir das maximum erreicht?
            if(curLineLen + cw > maxWidth && it != begin)
            {
                if(maxNumChars)
                    *maxNumChars = static_cast<unsigned>(std::distance(begin, it));
                return curLineLen;
            }
            curLineLen += cw;
        }
    }

    if(curLineLen > maxLineLen) // Letzte Zeile kann auch die längste sein und hat kein \n am Ende
        maxLineLen = curLineLen;

    if(maxNumChars)
        *maxNumChars = static_cast<unsigned>(std::distance(begin, end));

    return maxLineLen;
}

/**
 *  liefert die Länge einer Zeichenkette.
 *
 *  @param[in]     text   Der Text
 *  @param[in]     length Textlänge
 *  @param[in,out] max    In:  maximale Breite des Textes in Pixeln
 *                        Out: maximale Breite in Buchstaben der in "max"-Pixel reinpasst.
 *
 *  @return Breite des Textes in Pixeln
 */
unsigned short glArchivItem_Font::getWidth(const ucString& text, unsigned length, unsigned max_width, unsigned* maxNumChars) const
{
    if(length == 0)
        length = unsigned(text.length());

    return getWidthInternal(text.begin(), text.begin() + length, max_width, maxNumChars);
}

unsigned short glArchivItem_Font::getWidth(const std::string& text, unsigned length, unsigned max_width, unsigned* maxNumChars) const
{
    if(length == 0)
        length = unsigned(text.length());

    return getWidthInternal(text.begin(), text.begin() + length, max_width, maxNumChars);
}

/**
 *  @brief
 */
std::vector<std::string> glArchivItem_Font::WrapInfo::CreateSingleStrings(const std::string& text)
{
    RTTR_Assert(utf8::is_valid(text.begin(), text.end())); // Can only handle UTF-8 strings!

    std::vector<std::string> destStrings;
    if(positions.empty())
        return destStrings;

    destStrings.reserve(positions.size());
    unsigned curStart = positions.front();
    for(std::vector<unsigned>::const_iterator it = positions.begin() + 1; it != positions.end(); ++it)
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
glArchivItem_Font::WrapInfo glArchivItem_Font::GetWrapInfo(const std::string& text, const unsigned short primary_width, const unsigned short secondary_width)
{
    if(!fontNoOutline)
        initFont();

    RTTR_Assert(utf8::is_valid(text.begin(), text.end())); // Can only handle UTF-8 strings!
    
    // Current line width
    unsigned line_width = 0;
    // Width of current word
    unsigned word_width = 0;

    WrapInfo wi;
    // We start the first line at the first char (so wi.positions.size() == numLines)
    wi.positions.push_back(0);

    utf8Iterator it(text.begin(), text.begin(), text.end());
    utf8Iterator itEnd(text.end(), text.begin(), text.end());
    utf8Iterator itWordStart = it;

    const unsigned spaceWidth = CharWidth(' ');

    uint32_t curChar = 1;
    for(;; ++it)
    {
        curChar = (it != itEnd) ? *it : 0;
        // Word ended
        if(curChar == 0 || curChar == '\n' || curChar == ' ')
        {
            // Is the current word to long for the current line
            if(word_width + line_width > ((wi.positions.size() == 1) ? primary_width : secondary_width))
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
                        if(line_width + letter_width <= ((wi.positions.size() == 1) ? primary_width : secondary_width))
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
            }else if(curChar == '\n')
            {
                // If line break add new line (after all the word-breaking above)
                itWordStart = nextIt(it);
                if(itWordStart == itEnd)
                    break; // Reached end
                word_width = 0;
                line_width = 0;
                wi.positions.push_back(static_cast<unsigned>(itWordStart.base() - text.begin()));
            }
        }
        else
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
    utf8_mapping.clear();
    fontWithOutline.reset(dynamic_cast<glArchivItem_Bitmap*>(libsiedler2::getAllocator().create(libsiedler2::BOBTYPE_BITMAP_RLE)));
    fontNoOutline.reset(dynamic_cast<glArchivItem_Bitmap*>(libsiedler2::getAllocator().create(libsiedler2::BOBTYPE_BITMAP_RLE)));

    // first, we have to find how much chars we really have
    unsigned int numChars = 0;
    for(unsigned int i = 0; i < size(); ++i)
    {
        if(get(i))
            ++numChars;
    }

    if(numChars == 0)
        return;

    const unsigned numCharsPerLine = static_cast<unsigned>(std::sqrt(static_cast<double>(numChars)));
    // Calc lines required (rounding up)
    const unsigned numLines = (numChars + numCharsPerLine - 1) / numCharsPerLine;

    BOOST_CONSTEXPR_OR_CONST unsigned spacing = 1;
    unsigned w = (dx + spacing * 2) * numCharsPerLine + spacing * 2;
    unsigned h = (dy + spacing * 2) * numLines + spacing * 2;
    std::vector<unsigned char> bufferWithOutline(w * h * 4); // RGBA Puffer für alle Buchstaben
    std::vector<unsigned char> bufferNoOutline(w * h * 4); // RGBA Puffer für alle Buchstaben

    libsiedler2::ArchivItem_Palette* const palette = LOADER.GetPaletteN("colors");
    unsigned x = spacing;
    unsigned y = spacing;
    numChars = 0;
    for(unsigned int i = 0; i < size(); ++i)
    {
        const libsiedler2::ArchivItem_Bitmap_Player* c = dynamic_cast<const libsiedler2::ArchivItem_Bitmap_Player*>(get(i));
        if(!c)
            continue;

        if((numChars % numCharsPerLine) == 0 && numChars > 0)
        {
            y += dy + spacing * 2;
            x = spacing;
        }

        // Spezialpalette (blaue Spielerfarben sind Grau) verwenden, damit man per OpenGL einfärben kann!
        c->print(&bufferNoOutline.front(),   w, h, libsiedler2::FORMAT_RGBA, palette, 128, x, y, 0, 0, 0, 0, true);
        c->print(&bufferWithOutline.front(), w, h, libsiedler2::FORMAT_RGBA, palette, 128, x, y);

        CharInfo ci(x, y, std::min<unsigned short>(dx + 2, c->getWidth()));

        utf8_mapping[i] = ci;
        x += dx + spacing * 2;
        ++numChars;
    }

    fontNoOutline->create(  w, h, &bufferNoOutline.front(),   w, h, libsiedler2::FORMAT_RGBA, palette);
    fontWithOutline->create(w, h, &bufferWithOutline.front(), w, h, libsiedler2::FORMAT_RGBA, palette);

    // Set the placeholder for non-existant glyphs. Use '?' if possible (should always be)
    if(helpers::contains(utf8_mapping, '?'))
        placeHolder = utf8_mapping['?'];
    else
    {
        // Fall back to first glyph in map (kinda random, but should not happen anyway)
        LOG.writeToFile("Cannot find '?' glyph in font!");
        placeHolder = utf8_mapping.begin()->second;
    }

    /*ArchivInfo items;
    items.pushC(_font);
    libsiedler2::loader::WriteBMP((std::string("font") + std::string(getName()) + std::string(".bmp")).c_str(), LOADER.GetPaletteN("colors"), &items);*/
}
