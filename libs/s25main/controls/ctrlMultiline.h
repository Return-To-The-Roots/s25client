// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Window.h"
#include "ogl/FontStyle.h"
#include <utility>
#include <vector>

struct MouseCoords;
class glFont;

class ctrlMultiline : public Window
{
public:
    /// Breite der Scrollbar
    static const unsigned short SCROLLBAR_WIDTH = 20;
    static const unsigned short PADDING = 2;

    /// Creates a multiline control with automatic/transparent wrapping of long lines and automatic scrollbar
    /// Note: Using non-default font-formats may cause issues when the scrollbar is shown
    ctrlMultiline(Window* parent, unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
                  const glFont* font, FontStyle format);

    void resize(const Extent& newSize) override;
    void addString(const std::string& str, unsigned color, bool scroll = true);
    /// Deletes all lines
    void clear();
    unsigned getNumLines() const { return static_cast<unsigned>(lines.size()); }
    /// Gibt den index-ten Eintrag zurück
    const std::string& getLine(unsigned index) const { return lines[index].str; }
    void setLine(unsigned index, const std::string& str, unsigned color);
    /// Resizes the height such that the given number of lines can be shown
    void setNumVisibleLines(unsigned numLines);
    /// Return the currently used size including padding and the (possible) scrollbar (<=width,  <= height)
    Extent getContentSize() const;

    /// Schaltet Box ein und aus
    void showBackground(bool showBackground) { showBackground_ = showBackground; }
    /// (Dis-)allows a scrollbar. If scrollbar is disabled, text will be restricted by the current height and succeeding
    /// lines won't be shown
    void setScrollBarAllowed(bool allowed);

    bool msgLeftDown(const MouseCoords& mc) override;
    bool msgLeftUp(const MouseCoords& mc) override;
    bool msgWheelUp(const MouseCoords& mc) override;
    bool msgWheelDown(const MouseCoords& mc) override;
    bool msgMouseMove(const MouseCoords& mc) override;

protected:
    void drawImpl() override;

private:
    struct Line
    {
        std::string str;
        unsigned color;
        Line() noexcept : color(0) {}
        Line(std::string str, unsigned color) : str(std::move(str)), color(color) {}
    };

    unsigned getContentWidth() const;

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

    void recalcVisibleLines();
    void recalcWrappedLines();
};
