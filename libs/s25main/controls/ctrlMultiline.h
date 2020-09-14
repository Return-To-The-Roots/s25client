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

#pragma once

#include "Window.h"
#include "ogl/FontStyle.h"
#include <utility>
#include <vector>

class MouseCoords;
class glFont;

class ctrlMultiline : public Window
{
public:
    /// Breite der Scrollbar
    static const unsigned short SCROLLBAR_WIDTH = 20;
    static const unsigned short PADDING = 2;

    /// Creates a multiline control with automatic/transparent wrapping of long lines and automatic scrollbar
    /// Note: Using non-default font-formats may cause issues when the scrollbar is shown
    ctrlMultiline(Window* parent, unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc, const glFont* font,
                  FontStyle format);

    void Resize(const Extent& newSize) override;
    void AddString(const std::string& str, unsigned color, bool scroll = true);
    /// Deletes all lines
    void Clear();
    unsigned GetNumLines() const { return static_cast<unsigned>(lines.size()); }
    /// Gibt den index-ten Eintrag zurück
    const std::string& GetLine(unsigned index) const { return lines[index].str; }
    void SetLine(unsigned index, const std::string& str, unsigned color);
    /// Resizes the height such that the given number of lines can be shown
    void SetNumVisibleLines(unsigned numLines);
    /// Return the currently used size including padding and the (possible) scrollbar (<=width,  <= height)
    Extent GetContentSize() const;

    /// Schaltet Box ein und aus
    void ShowBackground(bool showBackground) { showBackground_ = showBackground; }
    /// (Dis-)allows a scrollbar. If scrollbar is disabled, text will be restricted by the current height and succeeding lines won't be
    /// shown
    void SetScrollBarAllowed(bool allowed);

    bool Msg_LeftDown(const MouseCoords& mc) override;
    bool Msg_LeftUp(const MouseCoords& mc) override;
    bool Msg_WheelUp(const MouseCoords& mc) override;
    bool Msg_WheelDown(const MouseCoords& mc) override;
    bool Msg_MouseMove(const MouseCoords& mc) override;

protected:
    void Draw_() override;

private:
    struct Line
    {
        std::string str;
        unsigned color;
        Line() : color(0) {}
        Line(std::string str, unsigned color) : str(std::move(str)), color(color) {}
    };

    unsigned GetContentWidth() const;

    TextureColor tc_;
    const glFont* font;
    FontStyle format_;
    bool showBackground_;
    bool scrollbarAllowed_;
    /// Lines to show
    std::vector<Line> lines;
    /// Actual lines to draw (possibly wrapped versions of lines)
    std::vector<Line> drawLines;
    /// Anzahl der Zeilen, die in das Control passen
    unsigned maxNumVisibleLines;
    /// Width of content as last calculated or 0
    mutable unsigned cachedContentWidth;

    void RecalcVisibleLines();
    void RecalcWrappedLines();
};
