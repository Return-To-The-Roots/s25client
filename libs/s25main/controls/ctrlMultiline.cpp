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

#include "ctrlMultiline.h"
#include "CollisionDetection.h"
#include "ctrlScrollBar.h"
#include "driver/MouseCoords.h"
#include "ogl/glFont.h"
#include <algorithm>

ctrlMultiline::ctrlMultiline(Window* parent, unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc, const glFont* font,
                             FontStyle format)
    : Window(parent, id, pos, size), tc_(tc), font(font), format_(format), showBackground_(true), scrollbarAllowed_(true),
      cachedContentWidth(0)
{
    RecalcVisibleLines();
    AddScrollBar(0, DrawPoint(size.x - SCROLLBAR_WIDTH, 0), Extent(SCROLLBAR_WIDTH, size.y), SCROLLBAR_WIDTH, tc, maxNumVisibleLines);
}

/**
 *  fügt eine Zeile hinzu.
 */
void ctrlMultiline::AddString(const std::string& str, unsigned color, bool scroll)
{
    lines.push_back(Line(str, color));
    RecalcWrappedLines();

    auto* scrollbar = GetCtrl<ctrlScrollBar>(0);
    if(scroll && scrollbar->GetScrollPos() + 1 + maxNumVisibleLines == lines.size())
        scrollbar->SetScrollPos(scrollbar->GetScrollPos() + 1);
}

void ctrlMultiline::Clear()
{
    lines.clear();
    RecalcWrappedLines();
}

/**
 *  zeichnet das Fenster.
 */
void ctrlMultiline::Draw_()
{
    if(showBackground_)
        Draw3D(Rect(GetDrawPos(), GetSize()), tc_, false);

    Window::Draw_();

    unsigned numVisibleLines = std::min<unsigned>(maxNumVisibleLines, drawLines.size());

    unsigned scrollbarPos = GetCtrl<ctrlScrollBar>(0)->GetScrollPos();
    DrawPoint curPos = GetDrawPos() + DrawPoint(PADDING, PADDING);
    for(unsigned i = 0; i < numVisibleLines; ++i)
    {
        font->Draw(curPos, drawLines[i + scrollbarPos].str, format_, drawLines[i + scrollbarPos].color);
        curPos.y += font->getHeight();
    }
}

void ctrlMultiline::RecalcVisibleLines()
{
    if(GetSize().y < 2 * PADDING)
        maxNumVisibleLines = 0;
    else
        maxNumVisibleLines = (GetSize().y - 2 * PADDING) / font->getHeight();
}

void ctrlMultiline::RecalcWrappedLines()
{
    drawLines.clear();
    cachedContentWidth = 0;
    // No space for a single line, or to narrow to even show the scrollbar -> Bail out
    if(maxNumVisibleLines == 0 || GetSize().x < 2 * PADDING + SCROLLBAR_WIDTH)
    {
        GetCtrl<ctrlScrollBar>(0)->SetRange(0);
        return;
    }
    // Calculate the wrap info for each real line (2nd pass if we need a scrollbar after breaking into lines)
    std::vector<glFont::WrapInfo> wrapInfos;
    wrapInfos.reserve(lines.size());
    bool needScrollBar = lines.size() > maxNumVisibleLines && scrollbarAllowed_;
    do
    {
        wrapInfos.clear();
        unsigned curNumLines = 0;
        unsigned maxTextWidth = GetSize().x - 2 * PADDING;
        if(needScrollBar)
            maxTextWidth -= SCROLLBAR_WIDTH;
        for(unsigned i = 0; i < lines.size(); i++)
        {
            wrapInfos.push_back(font->GetWrapInfo(lines[i].str, maxTextWidth, maxTextWidth));
            if(!needScrollBar)
            {
                curNumLines += wrapInfos[i].lines.size();
                if(curNumLines > maxNumVisibleLines)
                    break;
            }
        }
        // We are done, if we already knew we need a scrollbar (latest at 2nd pass)
        // or if we don't need it even after potentially breaking long lines
        if(needScrollBar || !scrollbarAllowed_ || curNumLines <= maxNumVisibleLines)
            break;
        else
            needScrollBar = true;
    } while(true); // Endless loop, exited at latest after 2nd pass

    // New create the actually drawn lines
    for(unsigned i = 0; i < wrapInfos.size(); i++)
    {
        // Special case: No break, just push the line as-is
        if(wrapInfos[i].lines.size() == 1u)
            drawLines.push_back(lines[i]);
        else
        {
            // Break it
            const std::vector<std::string> newLines = wrapInfos[i].CreateSingleStrings(lines[i].str);
            for(const std::string& line : newLines)
                drawLines.push_back(Line(line, lines[i].color));
        }
    }
    // If we don't have a scrollbar, restrict to maximum displayable lines
    if(!scrollbarAllowed_ && drawLines.size() > maxNumVisibleLines)
        drawLines.resize(maxNumVisibleLines);
    GetCtrl<ctrlScrollBar>(0)->SetRange(drawLines.size());
}

void ctrlMultiline::SetScrollBarAllowed(bool allowed)
{
    if(scrollbarAllowed_ != allowed)
    {
        scrollbarAllowed_ = allowed;
        RecalcWrappedLines();
    }
}

bool ctrlMultiline::Msg_LeftDown(const MouseCoords& mc)
{
    return GetCtrl<Window>(0)->Msg_LeftDown(mc);
}

bool ctrlMultiline::Msg_LeftUp(const MouseCoords& mc)
{
    return GetCtrl<Window>(0)->Msg_LeftUp(mc);
}
bool ctrlMultiline::Msg_WheelUp(const MouseCoords& mc)
{
    const Extent padding(PADDING, PADDING);
    if(IsPointInRect(mc.GetPos(), Rect(GetDrawPos() + DrawPoint(padding), GetSize() - padding * 2u)))
    {
        auto* scrollbar = GetCtrl<ctrlScrollBar>(0);
        scrollbar->Scroll(-3);
        return true;
    } else
        return false;
}

bool ctrlMultiline::Msg_WheelDown(const MouseCoords& mc)
{
    const Extent padding(PADDING, PADDING);
    if(IsPointInRect(mc.GetPos(), Rect(GetDrawPos() + DrawPoint(padding), GetSize() - padding * 2u)))
    {
        auto* scrollbar = GetCtrl<ctrlScrollBar>(0);
        scrollbar->Scroll(+3);
        return true;
    } else
        return false;
}

bool ctrlMultiline::Msg_MouseMove(const MouseCoords& mc)
{
    return GetCtrl<Window>(0)->Msg_MouseMove(mc);
}

void ctrlMultiline::Resize(const Extent& newSize)
{
    const Extent oldSize(GetSize());
    const unsigned oldMaxNumVisLines = maxNumVisibleLines;
    Window::Resize(newSize);

    RecalcVisibleLines();
    auto* scrollBar = GetCtrl<ctrlScrollBar>(0);
    scrollBar->SetPageSize(maxNumVisibleLines);
    scrollBar->SetHeight(GetSize().y);
    scrollBar->SetPos(DrawPoint(GetSize().x - SCROLLBAR_WIDTH, 0));
    // Recalc only if:
    // - we increased the size or decreased beyond content width
    // - scrollbar requirement has changed (e.g. now need one but did not need it before)
    const bool oldNeedScrollbar = oldMaxNumVisLines < drawLines.size();
    const bool newNeedScrollbar = maxNumVisibleLines < drawLines.size();
    if(oldNeedScrollbar != newNeedScrollbar || GetSize().x > oldSize.x || GetSize().x < GetContentWidth())
        RecalcWrappedLines();
}

/// Textzeile ersetzen. Klappt bestimmt nicht mit Scrollbar-Kram
void ctrlMultiline::SetLine(const unsigned index, const std::string& str, unsigned color)
{
    if(index < lines.size())
    {
        lines[index] = Line(str, color);
        RecalcWrappedLines();
    }
}

void ctrlMultiline::SetNumVisibleLines(unsigned numLines)
{
    SetHeight(numLines * font->getHeight() + 2 * PADDING);
}

Extent ctrlMultiline::GetContentSize() const
{
    return Extent(GetContentWidth(), std::min<unsigned>(GetSize().y, drawLines.size() * font->getHeight() + 2u * PADDING));
}

unsigned ctrlMultiline::GetContentWidth() const
{
    if(cachedContentWidth > 0)
        return cachedContentWidth;

    unsigned maxWidth = 0;
    unsigned addWidth = 2 * PADDING;
    if(drawLines.size() > maxNumVisibleLines)
        addWidth += SCROLLBAR_WIDTH;
    for(const Line& line : drawLines)
    {
        unsigned curWidth = font->getWidth(line.str) + addWidth;
        if(curWidth > maxWidth)
        {
            maxWidth = curWidth;
            if(maxWidth >= GetSize().x)
                return GetSize().x;
        }
    }
    cachedContentWidth = std::min<unsigned>(GetSize().x, maxWidth);
    return cachedContentWidth;
}
