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

#include "ctrlList.h"
#include "CollisionDetection.h"
#include "ctrlScrollBar.h"
#include "driver/MouseCoords.h"
#include "ogl/glFont.h"

ctrlList::ctrlList(Window* parent, unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc, const glFont* font)
    : Window(parent, id, pos, elMax(size, Extent(22, 4))), tc(tc), font(font), selection_(-1), mouseover(-1)
{
    pagesize = (GetSize().y - 4) / font->getHeight();

    AddScrollBar(0, DrawPoint(GetSize().x - 20, 0), Extent(20, GetSize().y), 20, tc, pagesize);
}

ctrlList::~ctrlList()
{
    DeleteAllItems();
}

void ctrlList::SetSelection(int selection)
{
    if(selection < 0)
        selection = -1;
    if(selection != selection_ && selection < static_cast<int>(lines.size()))
    {
        selection_ = selection;
        if(selection >= 0 && GetParent())
            GetParent()->Msg_ListSelectItem(GetID(), selection);
    }
}

bool ctrlList::Msg_MouseMove(const MouseCoords& mc)
{
    auto* scrollbar = GetCtrl<ctrlScrollBar>(0);

    mouseover = GetItemFromPos(mc.pos);
    // Wenn Maus in der Liste
    if(mouseover >= 0)
    {
        const std::string itemTxt = GetItemText(mouseover + scrollbar->GetScrollPos());
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

    auto itemIdx = GetItemFromPos(mc.pos);
    if(itemIdx >= 0)
    {
        tooltip_.HideTooltip();
        SetSelection(itemIdx + scrollbar->GetScrollPos());
        return true;
    }

    // Für die Scrollbar weiterleiten
    return scrollbar->Msg_LeftDown(mc);
}

bool ctrlList::Msg_RightDown(const MouseCoords& mc)
{
    auto* scrollbar = GetCtrl<ctrlScrollBar>(0);

    auto itemIdx = GetItemFromPos(mc.pos);
    if(itemIdx >= 0)
    {
        tooltip_.HideTooltip();
        SetSelection(itemIdx + scrollbar->GetScrollPos());
        return true;
    }

    // Für die Scrollbar weiterleiten
    return scrollbar->Msg_RightDown(mc);
}

bool ctrlList::Msg_LeftUp(const MouseCoords& mc)
{
    auto* scrollbar = GetCtrl<ctrlScrollBar>(0);

    // Wenn Maus in der Liste
    if(IsPointInRect(mc.GetPos(), GetListDrawArea()))
    {
        // Doppelklick? Dann noch einen extra Eventhandler aufrufen
        if(mc.dbl_click && GetParent() && selection_ >= 0)
            GetParent()->Msg_ListChooseItem(GetID(), selection_);

        return true;
    }

    // Für die Scrollbar weiterleiten
    return scrollbar->Msg_LeftUp(mc);
}

bool ctrlList::Msg_WheelUp(const MouseCoords& mc)
{
    // If mouse in list or scrollbar
    if(IsPointInRect(mc.GetPos(), GetFullDrawArea()))
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
    if(IsPointInRect(mc.GetPos(), GetFullDrawArea()))
    {
        auto* scrollbar = GetCtrl<ctrlScrollBar>(0);
        scrollbar->Scroll(+1);
        return true;
    }

    return false;
}

/**
 *  Zeichenmethode.
 *
 *  @return @p true bei Erfolg, @p false bei Fehler
 */
void ctrlList::Draw_()
{
    if(lines.empty())
        return;

    // Box malen
    Draw3D(Rect(GetDrawPos(), GetSize()), tc, false);

    // Scrolleiste zeichnen
    Window::Draw_();

    // Wieviele Linien anzeigen?
    unsigned show_lines = (pagesize > lines.size() ? unsigned(lines.size()) : pagesize);

    int scrollbarPos = GetCtrl<ctrlScrollBar>(0)->GetScrollPos();
    DrawPoint curPos = GetDrawPos() + DrawPoint(2, 2);
    // Listeneinträge zeichnen
    for(unsigned short i = 0; i < show_lines; ++i)
    {
        // Schwarze Markierung, wenn die Maus drauf ist
        if(i == mouseover)
            DrawRectangle(Rect(curPos, Extent(GetSize().x - 22, font->getHeight())), 0x80000000);

        // Text an sich
        font->Draw(curPos, lines[i + scrollbarPos], FontStyle{}, (selection_ == i + scrollbarPos ? 0xFFFFAA00 : COLOR_YELLOW),
                   GetSize().x - 22);
        curPos.y += font->getHeight();
    }
}

/**
 *  fügt eine Zeile hinzu.
 */
void ctrlList::AddString(const std::string& text)
{
    // lines-Array ggf vergrößern
    lines.push_back(text);

    GetCtrl<ctrlScrollBar>(0)->SetRange(static_cast<unsigned short>(lines.size()));
}

/**
 *  Verändert einen String
 */
void ctrlList::SetString(const std::string& text, const unsigned id)
{
    lines[id] = text;
}

/**
 *  löscht alle Items.
 */
void ctrlList::DeleteAllItems()
{
    lines.clear();
    selection_ = -1;
}

/**
 *  liefert den Wert einer Zeile.
 *
 *  @param[in] line Die Zeile
 *
 *  @return Text der Zeile
 */
const std::string& ctrlList::GetItemText(unsigned short line) const
{
    static const std::string EMPTY;
    if(line < lines.size())
        return lines[line];

    return EMPTY;
}

/**
 *  Größe ändern.
 *
 *  @param[in] width  Neue Breite
 *  @param[in] height Neue Höhe
 */
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

/**
 *  vertauscht zwei Zeilen
 */
void ctrlList::Swap(unsigned short first, unsigned short second)
{
    // Evtl Selection auf das jeweilige Element beibehalten?
    if(first == selection_)
        selection_ = second;
    else if(second == selection_)
        selection_ = first;

    // Strings vertauschen
    std::swap(lines[first], lines[second]);
}

/**
 *  entfernt eine Zeile
 */
void ctrlList::Remove(const unsigned short index)
{
    if(index < lines.size())
    {
        lines.erase(lines.begin() + index);

        // Keep current item selected
        if(selection_ > index)
            --selection_;
        else if(selection_ == index)
        {
            // Current item deleted -> clear selection
            selection_ = -1;
            if(index < GetNumLines())
                SetSelection(index); // select item now at deleted position
            else
                SetSelection(index - 1); // or previous if item at end deleted
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

int ctrlList::GetItemFromPos(const Position& pos) const
{
    const Rect listDrawArea = GetListDrawArea();
    if(IsPointInRect(pos, listDrawArea))
        return (pos.y - listDrawArea.getOrigin().y) / font->getHeight();
    else
        return -1;
}

Rect ctrlList::GetFullDrawArea() const
{
    return Rect(GetDrawPos() + DrawPoint(2, 2), GetSize() - Extent(2, 4));
}
