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
    pagesize = (GetSize().y - 4) / font->getHeight();

    AddScrollBar(0, DrawPoint(GetSize().x - 20, 0), Extent(20, GetSize().y), 20, tc, pagesize);
}

ctrlList::~ctrlList()
{
    DeleteAllItems();
}

void ctrlList::SetSelection(const boost::optional<unsigned>& selection)
{
    if(selection != selection_ && (!selection || *selection < lines.size()))
    {
        selection_ = selection;
        if(selection && GetParent())
            GetParent()->Msg_ListSelectItem(GetID(), *selection);
    }
}

bool ctrlList::Msg_MouseMove(const MouseCoords& mc)
{
    auto* scrollbar = GetCtrl<ctrlScrollBar>(0);

    mouseover_ = GetItemFromPos(mc.pos);
    // Wenn Maus in der Liste
    if(mouseover_)
    {
        const std::string itemTxt = GetItemText(*mouseover_);
        tooltip_.ShowTooltip((font->getWidth(itemTxt) > GetListDrawArea().getSize().x) ? itemTxt : "");
        return true;
    }

    tooltip_.HideTooltip();

    // Für die Scrollbar weiterleiten
    return scrollbar->Msg_MouseMove(mc);
}

bool ctrlList::Msg_LeftDown(const MouseCoords& mc)
{
    auto* scrollbar = GetCtrl<ctrlScrollBar>(0);

    const auto itemIdx = GetItemFromPos(mc.pos);
    if(itemIdx)
    {
        tooltip_.HideTooltip();
        SetSelection(*itemIdx);
        return true;
    }

    // Für die Scrollbar weiterleiten
    return scrollbar->Msg_LeftDown(mc);
}

bool ctrlList::Msg_RightDown(const MouseCoords& mc)
{
    auto* scrollbar = GetCtrl<ctrlScrollBar>(0);

    auto itemIdx = GetItemFromPos(mc.pos);
    if(itemIdx)
    {
        tooltip_.HideTooltip();
        SetSelection(*itemIdx);
        return true;
    }

    // Für die Scrollbar weiterleiten
    return scrollbar->Msg_RightDown(mc);
}

bool ctrlList::Msg_LeftUp(const MouseCoords& mc)
{
    auto* scrollbar = GetCtrl<ctrlScrollBar>(0);

    // Wenn Maus in der Liste
    if(IsPointInRect(mc.pos, GetListDrawArea()))
    {
        // Doppelklick? Dann noch einen extra Eventhandler aufrufen
        if(mc.dbl_click && GetParent() && selection_)
            GetParent()->Msg_ListChooseItem(GetID(), *selection_);

        return true;
    }

    // Für die Scrollbar weiterleiten
    return scrollbar->Msg_LeftUp(mc);
}

bool ctrlList::Msg_WheelUp(const MouseCoords& mc)
{
    // If mouse in list or scrollbar
    if(IsPointInRect(mc.pos, GetFullDrawArea()))
    {
        auto* scrollbar = GetCtrl<ctrlScrollBar>(0);
        scrollbar->Scroll(-1);
        return true;
    }

    return false;
}

bool ctrlList::Msg_WheelDown(const MouseCoords& mc)
{
    // If mouse in list
    if(IsPointInRect(mc.pos, GetFullDrawArea()))
    {
        auto* scrollbar = GetCtrl<ctrlScrollBar>(0);
        scrollbar->Scroll(+1);
        return true;
    }

    return false;
}

void ctrlList::Draw_()
{
    if(lines.empty())
        return;

    // Box malen
    Draw3D(Rect(GetDrawPos(), GetSize()), tc, false);

    // Scrolleiste zeichnen
    Window::Draw_();

    // Wieviele Linien anzeigen?
    const unsigned show_lines = (pagesize > lines.size() ? unsigned(lines.size()) : pagesize);

    const unsigned scrollbarPos = GetCtrl<ctrlScrollBar>(0)->GetScrollPos();
    DrawPoint curPos = GetDrawPos() + DrawPoint(2, 2);
    // Listeneinträge zeichnen
    for(unsigned i = 0; i < show_lines; ++i)
    {
        // Schwarze Markierung, wenn die Maus drauf ist
        if(i + scrollbarPos == mouseover_)
            DrawRectangle(Rect(curPos, Extent(GetSize().x - 22, font->getHeight())), 0x80000000);

        // Text an sich
        font->Draw(curPos, lines[i + scrollbarPos], FontStyle{},
                   (selection_ == i + scrollbarPos ? 0xFFFFAA00 : COLOR_YELLOW), GetSize().x - 22);
        curPos.y += font->getHeight();
    }
}

void ctrlList::AddItem(const std::string& text)
{
    // lines-Array ggf vergrößern
    lines.push_back(text);

    GetCtrl<ctrlScrollBar>(0)->SetRange(static_cast<unsigned short>(lines.size()));
}

void ctrlList::SetItemText(const unsigned id, const std::string& text)
{
    lines.at(id) = text;
}

void ctrlList::DeleteAllItems()
{
    lines.clear();
    selection_ = boost::none;
}

const std::string& ctrlList::GetItemText(unsigned line) const
{
    RTTR_Assert(line < lines.size());
    return lines[line];
}

const std::string& ctrlList::GetSelItemText() const
{
    static const std::string EMPTY;
    if(selection_)
        return GetItemText(*selection_);
    else
        return EMPTY;
}

void ctrlList::Resize(const Extent& newSize)
{
    auto* scrollbar = GetCtrl<ctrlScrollBar>(0);
    scrollbar->SetPos(DrawPoint(newSize.x - 20, 0));
    scrollbar->Resize(Extent(20, newSize.y));

    pagesize = (newSize.y - 4) / font->getHeight();

    scrollbar->SetPageSize(pagesize);

    // If the size was enlarged we have to check that we don't try to
    // display more lines than present
    if(newSize.y > GetSize().y)
        while(lines.size() - scrollbar->GetScrollPos() < pagesize && scrollbar->GetScrollPos() > 0)
            scrollbar->SetScrollPos(scrollbar->GetScrollPos() - 1);

    Window::Resize(newSize);
}

void ctrlList::Swap(unsigned first, unsigned second)
{
    // Evtl Selection auf das jeweilige Element beibehalten?
    if(first == selection_)
        selection_ = second;
    else if(second == selection_)
        selection_ = first;

    // Strings vertauschen
    std::swap(lines[first], lines[second]);
}

void ctrlList::Remove(const unsigned index)
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
            selection_ = boost::none;
            if(index < GetNumLines())
                SetSelection(index); // select item now at deleted position
            else if(index > 0u)
                SetSelection(index - 1u); // or previous if item at end deleted
        }
    }
}

Rect ctrlList::GetListDrawArea() const
{
    // Full area minus scrollbar
    Rect result = GetFullDrawArea();
    Extent size = result.getSize();
    size.x -= 20;
    result.setSize(size);
    return result;
}

boost::optional<unsigned> ctrlList::GetItemFromPos(const Position& pos) const
{
    const Rect listDrawArea = GetListDrawArea();
    if(!IsPointInRect(pos, listDrawArea))
        return boost::none;
    const unsigned itemIdx =
      (pos.y - listDrawArea.getOrigin().y) / font->getHeight() + GetCtrl<ctrlScrollBar>(0)->GetScrollPos();
    if(itemIdx >= GetNumLines())
        return boost::none;
    return itemIdx;
}

Rect ctrlList::GetFullDrawArea() const
{
    return Rect(GetDrawPos() + DrawPoint(2, 2), GetSize() - Extent(2, 4));
}
