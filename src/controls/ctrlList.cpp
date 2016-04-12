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

///////////////////////////////////////////////////////////////////////////////
// Header
#include "defines.h" // IWYU pragma: keep
#include "ctrlList.h"
#include "ctrlScrollBar.h"
#include "CollisionDetection.h"
#include "WindowManager.h"
#include "ogl/glArchivItem_Font.h"

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

ctrlList::ctrlList(Window* parent,
                   unsigned int id,
                   unsigned short x,
                   unsigned short y,
                   unsigned short width,
                   unsigned short height,
                   TextureColor tc,
                   glArchivItem_Font* font)
    : Window(x, y, id, parent, width, height),
      tc(tc), font(font), selection_(-1), mouseover(-1)
{
    pagesize = (height - 4) / font->getHeight();

    AddScrollBar(0, width - 20, 0, 20, height, 20, tc, pagesize);
}

ctrlList::~ctrlList()
{
    DeleteAllItems();
}

bool ctrlList::Msg_MouseMove(const MouseCoords& mc)
{
    ctrlScrollBar* scrollbar = GetCtrl<ctrlScrollBar>(0);

    // Wenn Maus in der Liste
    if(Coll(mc.x, mc.y, GetX() + 2, GetY() + 2, width_ - 22, height_ - 4))
    {
        // Neue Selektierung
        mouseover = (mc.y - (GetY() + 2) ) / font->getHeight();
        WINDOWMANAGER.SetToolTip(this,
                                         ((font->getWidth(GetItemText(mouseover)) > width_ - 22) ?
                                          GetItemText(mouseover) : ""));
        return true;
    }

    // Mouse-Over deaktivieren und Tooltip entfernen
    mouseover = -1;
    WINDOWMANAGER.SetToolTip(this, "");

    // Für die Scrollbar weiterleiten
    return scrollbar->Msg_MouseMove(mc);
}

bool ctrlList::Msg_LeftDown(const MouseCoords& mc)
{
    ctrlScrollBar* scrollbar = GetCtrl<ctrlScrollBar>(0);

    // Wenn Maus in der Liste
    if(Coll(mc.x, mc.y, GetX() + 2, GetY() + 2, width_ - 22, height_ - 4))
    {
        // Tooltip löschen, sonst bleibt er ewig
        WINDOWMANAGER.SetToolTip(this, "");

        // aktuellen Eintrag selektieren
        selection_ = mouseover + scrollbar->GetPos();

        if(parent_) parent_->Msg_ListSelectItem(id_, selection_);

        return true;
    }

    // Für die Scrollbar weiterleiten
    return scrollbar->Msg_LeftDown(mc);
}

bool ctrlList::Msg_RightDown(const MouseCoords& mc)
{
    ctrlScrollBar* scrollbar = GetCtrl<ctrlScrollBar>(0);

    // Wenn Maus in der Liste
    if(Coll(mc.x, mc.y, GetX() + 2, GetY() + 2, width_ - 22, height_ - 4))
    {
        // Tooltip löschen, sonst bleibt er ewig
        WINDOWMANAGER.SetToolTip(this, "");

        // aktuellen Eintrag selektieren
        selection_ = mouseover + scrollbar->GetPos();
        parent_->Msg_ListSelectItem(id_, selection_);

        return true;
    }

    // Für die Scrollbar weiterleiten
    return scrollbar->Msg_RightDown(mc);
}

bool ctrlList::Msg_LeftUp(const MouseCoords& mc)
{
    ctrlScrollBar* scrollbar = GetCtrl<ctrlScrollBar>(0);

    // Wenn Maus in der Liste
    if(Coll(mc.x, mc.y, GetX() + 2, GetY() + 2, width_ - 22, height_ - 4))
    {
        // Doppelklick? Dann noch einen extra Eventhandler aufrufen
        if(mc.dbl_click && parent_ && selection_ >= 0)
            parent_->Msg_ListChooseItem(id_, selection_);

        return true;
    }

    // Für die Scrollbar weiterleiten
    return scrollbar->Msg_LeftUp(mc);
}

bool ctrlList::Msg_WheelUp(const MouseCoords& mc)
{
    // Forward to ScrollBar
    ctrlScrollBar* scrollbar = GetCtrl<ctrlScrollBar>(0);

    // If mouse in list or scrollbar
    if(Coll(mc.x, mc.y, GetX() + 2, GetY() + 2, width_ - /*2*/2, height_ - 4))
    {
        // Simulate Button Click
        scrollbar->Msg_ButtonClick(0);
        return true;
    }

    return false;
}

bool ctrlList::Msg_WheelDown(const MouseCoords& mc)
{
    // Forward to ScrollBar
    ctrlScrollBar* scrollbar = GetCtrl<ctrlScrollBar>(0);

    // If mouse in list
    if(Coll(mc.x, mc.y, GetX() + 2, GetY() + 2, width_ - /*2*/2, height_ - 4))
    {
        // Simulate Button Click
        scrollbar->Msg_ButtonClick(1);
        return true;
    }

    return false;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Zeichenmethode.
 *
 *  @return @p true bei Erfolg, @p false bei Fehler
 *
 *  @author OLiver
 */
bool ctrlList::Draw_()
{
    ctrlScrollBar* scrollbar = GetCtrl<ctrlScrollBar>(0);

    // Box malen
    Draw3D(GetX(), GetY(), width_, height_, tc, 2);

    // Scrolleiste zeichnen
    DrawControls();

    // Wieviele Linien anzeigen?
    unsigned show_lines = (pagesize > lines.size() ? unsigned(lines.size()) : pagesize);

    // Listeneinträge zeichnen
    for(unsigned short i = 0; i < show_lines; ++i)
    {
        // Schwarze Markierung, wenn die Maus drauf ist
        if(i == mouseover)
            DrawRectangle(GetX() + 2, GetY() + 2 + i * font->getHeight(), width_ - 22, font->getHeight(), 0x80000000);

        // Text an sich
        font->Draw(GetX() + 2, GetY() + 2 + i * font->getHeight(), lines[i + scrollbar->GetPos()], 0, (selection_ == i + scrollbar->GetPos() ? 0xFFFFAA00 : 0xFFFFFF00), 0, width_ - 22);
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  fügt eine Zeile hinzu.
 *
 *  @author FloSoft
 */
void ctrlList::AddString(const std::string& text)
{
    // lines-Array ggf vergrößern
    lines.push_back(text);

    GetCtrl<ctrlScrollBar>(0)->SetRange(static_cast<unsigned short>(lines.size()));
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Verändert einen String
 *
 *  @author OLiver
 */
void ctrlList::SetString(const std::string& text, const unsigned id)
{
    lines[id] = text;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  löscht alle Items.
 *
 *  @author FloSoft
 */
void ctrlList::DeleteAllItems()
{
    lines.clear();
    selection_ = -1;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  liefert den Wert einer Zeile.
 *
 *  @param[in] line Die Zeile
 *
 *  @return Text der Zeile
 *
 *  @author OLiver
 */
const std::string& ctrlList::GetItemText(unsigned short line) const
{
    if(line < lines.size())
        return lines.at(line);

    return EMPTY_STRING;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Größe ändern.
 *
 *  @param[in] width  Neue Breite
 *  @param[in] height Neue Höhe
 *
 *  @author OLiver
 */
void ctrlList::Resize_(unsigned short width, unsigned short height)
{
    ctrlScrollBar* scrollbar = GetCtrl<ctrlScrollBar>(0);
    scrollbar->Move(width - 20, 0);
    scrollbar->Resize(20, height);

    pagesize = (height - 4) / font->getHeight();

    scrollbar->SetPageSize(pagesize);

    // If the size was enlarged we have to check that we don't try to
    // display more lines than present
    if(height > this->height_)
        while(lines.size() - scrollbar->GetPos() < pagesize
                && scrollbar->GetPos() > 0)
            scrollbar->SetPos(scrollbar->GetPos() - 1);

}

///////////////////////////////////////////////////////////////////////////////
/**
 *  vertauscht zwei Zeilen
 *
 *  @author OLiver
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

///////////////////////////////////////////////////////////////////////////////
/**
 *  entfernt eine Zeile
 *
 *  @author OLiver
 */
void ctrlList::Remove(const unsigned short index)
{
    if(index < lines.size())
    {
        lines.erase(lines.begin() + index);

        // Ggf. selection korrigieren
        if(selection_)
            --selection_;
        else
            selection_ = -1;
    }
}
