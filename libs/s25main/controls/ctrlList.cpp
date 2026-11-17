// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ctrlList.h"
#include "CollisionDetection.h"
#include "ctrlScrollBar.h"
#include "driver/MouseCoords.h"
#include "ogl/glFont.h"

ctrlList::ctrlList(Window* parent, unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
                   const glFont* font)
    : Window(parent, id, pos, elMax(size, Extent(22, 4))), tc(tc), font(font)
{
    pagesize = (getSize().y - 4) / font->getHeight();

    addScrollBar(0, DrawPoint(getSize().x - 20, 0), Extent(20, getSize().y), 20, tc, pagesize);
}

ctrlList::~ctrlList()
{
    deleteAllItems();
}

void ctrlList::setSelection(const std::optional<unsigned>& selection)
{
    if(selection != selection_ && (!selection || *selection < lines.size()))
    {
        selection_ = selection;
        if(selection && getParent())
            getParent()->msgListSelectItem(getID(), *selection);
    }
}

bool ctrlList::msgMouseMove(const MouseCoords& mc)
{
    auto* scrollbar = getCtrl<ctrlScrollBar>(0);

    mouseover_ = getItemFromPos(mc.pos);
    // Wenn Maus in der Liste
    if(mouseover_)
    {
        const std::string itemTxt = getItemText(*mouseover_);
        tooltip_.showTooltip((font->getWidth(itemTxt) > getListDrawArea().getSize().x) ? itemTxt : "");
        return true;
    }

    tooltip_.hideTooltip();

    // Für die Scrollbar weiterleiten
    return scrollbar->msgMouseMove(mc);
}

bool ctrlList::msgLeftDown(const MouseCoords& mc)
{
    auto* scrollbar = getCtrl<ctrlScrollBar>(0);

    const auto itemIdx = getItemFromPos(mc.pos);
    if(itemIdx)
    {
        tooltip_.hideTooltip();
        setSelection(*itemIdx);
        return true;
    }

    // Für die Scrollbar weiterleiten
    return scrollbar->msgLeftDown(mc);
}

bool ctrlList::msgRightDown(const MouseCoords& mc)
{
    auto* scrollbar = getCtrl<ctrlScrollBar>(0);

    auto itemIdx = getItemFromPos(mc.pos);
    if(itemIdx)
    {
        tooltip_.hideTooltip();
        setSelection(*itemIdx);
        return true;
    }

    // Für die Scrollbar weiterleiten
    return scrollbar->msgRightDown(mc);
}

bool ctrlList::msgLeftUp(const MouseCoords& mc)
{
    auto* scrollbar = getCtrl<ctrlScrollBar>(0);

    // Wenn Maus in der Liste
    if(isPointInRect(mc.pos, getListDrawArea()))
    {
        // Doppelklick? Dann noch einen extra Eventhandler aufrufen
        if(mc.dbl_click && getParent() && selection_)
            getParent()->msgListChooseItem(getID(), *selection_);

        return true;
    }

    // Für die Scrollbar weiterleiten
    return scrollbar->msgLeftUp(mc);
}

bool ctrlList::msgWheelUp(const MouseCoords& mc)
{
    // If mouse in list or scrollbar
    if(isPointInRect(mc.pos, getFullDrawArea()))
    {
        auto* scrollbar = getCtrl<ctrlScrollBar>(0);
        scrollbar->scroll(-1);
        return true;
    }

    return false;
}

bool ctrlList::msgWheelDown(const MouseCoords& mc)
{
    // If mouse in list
    if(isPointInRect(mc.pos, getFullDrawArea()))
    {
        auto* scrollbar = getCtrl<ctrlScrollBar>(0);
        scrollbar->scroll(+1);
        return true;
    }

    return false;
}

void ctrlList::drawImpl()
{
    if(lines.empty())
        return;

    // Box malen
    draw3D(Rect(getDrawPos(), getSize()), tc, false);

    // Scrolleiste zeichnen
    Window::drawImpl();

    // Wieviele Linien anzeigen?
    const unsigned showLines = (pagesize > lines.size() ? unsigned(lines.size()) : pagesize);

    const unsigned scrollbarPos = getCtrl<ctrlScrollBar>(0)->getScrollPos();
    DrawPoint curPos = getDrawPos() + DrawPoint(2, 2);
    // Listeneinträge zeichnen
    for(unsigned i = 0; i < showLines; ++i)
    {
        // Schwarze Markierung, wenn die Maus drauf ist
        if(i + scrollbarPos == mouseover_)
            drawRectangle(Rect(curPos, Extent(getSize().x - 22, font->getHeight())), 0x80000000);

        // Text an sich
        font->draw(curPos, lines[i + scrollbarPos], FontStyle{},
                   (selection_ == i + scrollbarPos ? 0xFFFFAA00 : COLOR_YELLOW), getSize().x - 22);
        curPos.y += font->getHeight();
    }
}

void ctrlList::addItem(const std::string& text)
{
    // lines-Array ggf vergrößern
    lines.push_back(text);

    getCtrl<ctrlScrollBar>(0)->setRange(static_cast<unsigned short>(lines.size()));
}

void ctrlList::setItemText(const unsigned id, const std::string& text)
{
    lines.at(id) = text;
}

void ctrlList::deleteAllItems()
{
    lines.clear();
    selection_ = std::nullopt;
}

const std::string& ctrlList::getItemText(unsigned line) const
{
    RTTR_Assert(line < lines.size());
    return lines[line];
}

const std::string& ctrlList::getSelItemText() const
{
    static const std::string EMPTY;
    if(selection_)
        return getItemText(*selection_);
    else
        return EMPTY;
}

void ctrlList::resize(const Extent& newSize)
{
    auto* scrollbar = getCtrl<ctrlScrollBar>(0);
    scrollbar->setPos(DrawPoint(newSize.x - 20, 0));
    scrollbar->resize(Extent(20, newSize.y));

    pagesize = (newSize.y - 4) / font->getHeight();

    scrollbar->setPageSize(pagesize);

    // If the size was enlarged we have to check that we don't try to
    // display more lines than present
    if(newSize.y > getSize().y)
        while(lines.size() - scrollbar->getScrollPos() < pagesize && scrollbar->getScrollPos() > 0)
            scrollbar->setScrollPos(scrollbar->getScrollPos() - 1);

    Window::resize(newSize);
}

void ctrlList::swap(unsigned first, unsigned second)
{
    // Evtl Selection auf das jeweilige Element beibehalten?
    if(first == selection_)
        selection_ = second;
    else if(second == selection_)
        selection_ = first;

    // Strings vertauschen
    std::swap(lines[first], lines[second]);
}

void ctrlList::remove(const unsigned index)
{
    if(index < lines.size())
    {
        lines.erase(lines.begin() + index);
        if(!selection_)
            return;

        // Keep current item selected
        if(*selection_ > index)
            --*selection_;
        else if(*selection_ == index)
        {
            // Current item deleted -> clear selection
            selection_ = std::nullopt;
            if(index < getNumLines())
                setSelection(index); // select item now at deleted position
            else if(index > 0u)
                setSelection(index - 1u); // or previous if item at end deleted
        }
    }
}

Rect ctrlList::getListDrawArea() const
{
    // Full area minus scrollbar
    Rect result = getFullDrawArea();
    Extent size = result.getSize();
    size.x -= 20;
    result.setSize(size);
    return result;
}

std::optional<unsigned> ctrlList::getItemFromPos(const Position& pos) const
{
    const Rect listDrawArea = getListDrawArea();
    if(!isPointInRect(pos, listDrawArea))
        return std::nullopt;
    const unsigned itemIdx =
      (pos.y - listDrawArea.getOrigin().y) / font->getHeight() + getCtrl<ctrlScrollBar>(0)->getScrollPos();
    if(itemIdx >= getNumLines())
        return std::nullopt;
    return itemIdx;
}

Rect ctrlList::getFullDrawArea() const
{
    return Rect(getDrawPos() + DrawPoint(2, 2), getSize() - Extent(2, 4));
}
