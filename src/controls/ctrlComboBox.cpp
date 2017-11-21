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
#include "ctrlComboBox.h"
#include "CollisionDetection.h"
#include "Loader.h"
#include "ctrlButton.h"
#include "ctrlList.h"
#include "driver/MouseCoords.h"
#include "ogl/FontStyle.h"
#include "ogl/SoundEffectItem.h"
#include "ogl/glArchivItem_Font.h"

ctrlComboBox::ctrlComboBox(Window* parent, unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc, glArchivItem_Font* font,
                           unsigned short max_list_height, bool readonly)
    : Window(parent, id, pos, size), tc(tc), font(font), max_list_height(max_list_height), readonly(readonly), last_show(false)
{
    ctrlList* liste = AddList(0, DrawPoint(0, size.y), Extent(size.x, 4), tc, font);

    // Liste am Anfang nicht anzeigen
    liste->SetVisible(false);

    if(!readonly)
        AddImageButton(1, DrawPoint(size.x - size.y, 0), size, tc, LOADER.GetImageN("io", 34));

    Resize(size);
}

/**
 *  Größe verändern
 */
void ctrlComboBox::Resize(const Extent& newSize)
{
    Window::Resize(newSize);

    ctrlButton* button = GetCtrl<ctrlButton>(1);
    if(button)
    {
        button->SetPos(DrawPoint(newSize.x - newSize.y, 0));
        button->Resize(Extent(newSize.y, newSize.y));
    }

    ctrlList* list = GetCtrl<ctrlList>(0);

    Extent listSize(newSize.x, 4);

    // Langsam die Höhe der maximalen annähern
    for(unsigned i = 0; i < list->GetNumLines(); ++i)
    {
        // zu große geworden?
        listSize.y += font->getHeight();
        unsigned short scaledMaxHeight = ScaleIf(Extent(0, max_list_height)).y;

        if(listSize.y > scaledMaxHeight)
        {
            // kann nicht mal ein Item aufnehmen, dann raus
            if(i == 0)
                return;

            // Höhe um eins erniedrigen, damits wieder kleiner ist als die maximale
            listSize.y -= font->getHeight();
            break;
        }
    }

    list->SetPos(DrawPoint(0, newSize.y));
    list->Resize(listSize);
}

void ctrlComboBox::Msg_PaintAfter()
{
    // Liste erst jetzt malen, damit sie den Rest überdeckt
    GetCtrl<ctrlList>(0)->Draw();
}

bool ctrlComboBox::Msg_MouseMove(const MouseCoords& mc)
{
    // Für Button und Liste weiterleiten
    return RelayMouseMessage(&Window::Msg_MouseMove, mc);
}

bool ctrlComboBox::Msg_LeftDown(const MouseCoords& mc)
{
    ctrlList* list = GetCtrl<ctrlList>(0);

    // Irgendwo anders hingeklickt --> Liste ausblenden
    if(!readonly && !IsPointInRect(mc.GetPos(), GetFullDrawRect(list)))
    {
        // Liste wieder ausblenden
        ShowList(false);
        return false;
    }

    if(!readonly && IsPointInRect(mc.GetPos(), GetDrawRect()))
    {
        // Liste wieder ein/ausblenden
        ShowList(!list->IsVisible());
        return true;
    }

    // Für Button und Liste weiterleiten
    return RelayMouseMessage(&Window::Msg_LeftDown, mc);
}

bool ctrlComboBox::Msg_LeftUp(const MouseCoords& mc)
{
    // Für Button und Liste weiterleiten
    return RelayMouseMessage(&Window::Msg_LeftUp, mc);
}

bool ctrlComboBox::Msg_RightDown(const MouseCoords& mc)
{
    ctrlList* list = GetCtrl<ctrlList>(0);

    // Für Button und Liste weiterleiten (und danach erst schließen)
    bool ret = RelayMouseMessage(&Window::Msg_RightDown, mc);

    // Clicked on list -> close it
    if(!readonly && IsPointInRect(mc.GetPos(), list->GetDrawRect()))
    {
        // Liste wieder ausblenden
        ShowList(false);
    }

    return ret;
}

bool ctrlComboBox::Msg_WheelUp(const MouseCoords& mc)
{
    if(readonly)
        return false;

    ctrlList* list = GetCtrl<ctrlList>(0);
    if(list->IsVisible() && IsPointInRect(mc.GetPos(), list->GetDrawRect()))
        return RelayMouseMessage(&Window::Msg_WheelUp, mc);

    if(IsPointInRect(mc.GetPos(), GetDrawRect()))
    {
        // Scrolled without list opened
        if(list->GetSelection() > 0)
            list->SetSelection(list->GetSelection() - 1);
        return true;
    }

    return false;
}

bool ctrlComboBox::Msg_WheelDown(const MouseCoords& mc)
{
    if(readonly)
        return false;

    ctrlList* list = GetCtrl<ctrlList>(0);

    if(list->IsVisible() && IsPointInRect(mc.GetPos(), list->GetDrawRect()))
    {
        // Scrolled in opened list ->
        return RelayMouseMessage(&Window::Msg_WheelDown, mc);
    }

    if(IsPointInRect(mc.GetPos(), GetDrawRect()))
    {
        // Scrolled without list opened
        if(list->GetSelection() + 1 < list->GetNumLines())
            Msg_ListSelectItem(GetID(), list->GetSelection() + 1);
        return true;
    }

    return false;
}

Rect ctrlComboBox::GetFullDrawRect(const ctrlList* list)
{
    Rect myRect = GetDrawRect();
    myRect.bottom = list->GetDrawRect().bottom;
    return myRect;
}

void ctrlComboBox::Msg_ListSelectItem(const unsigned ctrl_id, const int selection)
{
    // Liste wieder ausblenden
    ShowList(false);

    ctrlList* list = GetCtrl<ctrlList>(0);

    // ist in der Liste überhaupt was drin?
    if(selection != selectionOnListOpen && list->GetNumLines() > 0)
    {
        // Nachricht an übergeordnetes Fenster verschicken
        GetParent()->Msg_ComboSelectItem(GetID(), selection);
    }
}

/**
 *  fügt einen String zur Liste hinzu.
 */
void ctrlComboBox::AddString(const std::string& text)
{
    GetCtrl<ctrlList>(0)->AddString(text);
    Resize(GetSize());
}

/**
 *  löscht alle Items der Liste.
 */
void ctrlComboBox::DeleteAllItems()
{
    GetCtrl<ctrlList>(0)->DeleteAllItems();
    Resize(GetSize());
}

/**
 *  wählt ein Item aus
 */
void ctrlComboBox::SetSelection(unsigned short selection)
{
    // Avoid sending the change method when this is invoked intentionally
    selectionOnListOpen = selection;
    GetCtrl<ctrlList>(0)->SetSelection(selection);
}

/**
 *  zeichnet das Fenster.
 */
void ctrlComboBox::Draw_()
{
    ctrlList* liste = GetCtrl<ctrlList>(0);

    // Box
    Draw3D(Rect(GetDrawPos(), GetSize()), tc, 2);

    // Namen des selektierten Strings in der Box anzeigen
    if(liste->GetNumLines() > 0)
        font->Draw(GetDrawPos() + DrawPoint(2, GetSize().y / 2), liste->GetSelItemText(), FontStyle::VCENTER, COLOR_YELLOW, 0,
                   GetSize().x - 2 - GetSize().y, "");

    // Male restliche Controls per Hand, denn ein einfaches DrawControls() würde
    // auch die Liste malen, die bei Msg_PaintAfter() sowieso gemalt wird.
    ctrlButton* button = GetCtrl<ctrlButton>(1);
    if(button)
        button->Draw();
}

/**
 *  blendet die Liste ein oder aus.
 */
void ctrlComboBox::ShowList(bool show)
{
    if(last_show == show)
        return;

    last_show = show;

    ctrlList* liste = GetCtrl<ctrlList>(0);

    // Liste entsprechend
    liste->SetVisible(show);

    // Pfeilbutton entsprechend
    GetCtrl<ctrlButton>(1)->SetChecked(show);

    // Region sperren für die Liste, oder freigeben
    if(show)
    {
        GetParent()->LockRegion(this, liste->GetDrawRect());
        selectionOnListOpen = GetSelection();
    } else
    {
        GetParent()->FreeRegion(this);
    }

    LOADER.GetSoundN("sound", 113)->Play(255, false);
}
