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
#include "Loader.h"
#include "ctrlComboBox.h"
#include "ctrlButton.h"
#include "ctrlList.h"
#include "CollisionDetection.h"
#include "driver/src/MouseCoords.h"
#include "ogl/glArchivItem_Font.h"
#include "ogl/glArchivItem_Sound.h"

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

ctrlComboBox::ctrlComboBox(Window* parent,
                           unsigned int id,
                           unsigned short x,
                           unsigned short y,
                           unsigned short width,
                           unsigned short height,
                           TextureColor tc,
                           glArchivItem_Font* font,
                           unsigned short max_list_height,
                           bool readonly)
    : Window(x, y, id, parent, width, height),
      tc(tc), font(font), max_list_height(max_list_height), readonly(readonly), last_show(false)
{
    ctrlList* liste = AddList(0, 0, height, width, 4, tc, font);

    // Liste am Anfang nicht anzeigen
    liste->SetVisible(false);

    if(!readonly)
        AddImageButton(1, width - height, 0, height, height, tc, LOADER.GetImageN("io", 34));

    Resize_(width, height);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Größe verändern
 *
 *  @author Divan
 *  @author OLiver
 */
void ctrlComboBox::Resize_(unsigned short width, unsigned short height)
{
    ctrlButton* button = GetCtrl<ctrlButton>(1);
    if(button)
    {
        button->Move(width - height, 0);
        button->Resize(height, height);
    }

    ctrlList* list = GetCtrl<ctrlList>(0);

    unsigned short list_height = 4;

    // Langsam die Höhe der maximalen annähern
    for(unsigned int i = 0; i < list->GetLineCount(); ++i)
    {
        // zu große geworden?
        list_height += font->getHeight();

        if(list_height > (scale_ ? ScaleY(max_list_height) : max_list_height))
        {
            // kann nicht mal ein Item aufnehmen, dann raus
            if(i == 0)
                return;

            // Höhe um eins erniedrigen, damits wieder kleiner ist als die maximale
            list_height -= font->getHeight();;

            break;
        }
    }

    list->Move(0, height);
    list->Resize(width, list_height);
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
    if(!readonly && !Coll(mc.x, mc.y, GetX(), GetY(), width_, height_ + list->GetHeight()))
    {
        // Liste wieder ausblenden
        ShowList(false);
        return false;
    }

    if(!readonly && Coll(mc.x, mc.y, GetX(), GetY(), width_, height_))
    {
        // Liste wieder ein/ausblenden
        ShowList(!list->GetVisible());
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
    if(!readonly && Coll(mc.x, mc.y, GetX(), GetY() + height_, width_, height_ + list->GetHeight()))
    {
        // Liste wieder ausblenden
        ShowList(false);
    }

    return ret;
}

bool ctrlComboBox::Msg_WheelUp(const MouseCoords& mc)
{
    ctrlList* list = GetCtrl<ctrlList>(0);

    if(!readonly && Coll(mc.x, mc.y, GetX(), GetY() + height_, width_, height_ + list->GetHeight()) && list->GetVisible())
    {
        // Scrolled in opened list ->
        return RelayMouseMessage(&Window::Msg_WheelUp, mc);
    }

    if(!readonly && Coll(mc.x, mc.y, GetX(), GetY(), width_, height_))
    {
        // Scrolled without list opened
        if (list->GetSelection() > 0)
            Msg_ListSelectItem(GetID(), list->GetSelection() - 1);
        return true;
    }

    return false;
}

bool ctrlComboBox::Msg_WheelDown(const MouseCoords& mc)
{
    ctrlList* list = GetCtrl<ctrlList>(0);

    if(!readonly && Coll(mc.x, mc.y, GetX(), GetY() + height_, width_, height_ + list->GetHeight()) && list->GetVisible())
    {
        // Scrolled in opened list ->
        return RelayMouseMessage(&Window::Msg_WheelDown, mc);
    }

    if(!readonly && Coll(mc.x, mc.y, GetX(), GetY(), width_, height_))
    {
        // Scrolled without list opened
        if (list->GetSelection() + 1 < list->GetLineCount())
            Msg_ListSelectItem(GetID(), list->GetSelection() + 1);
        return true;
    }

    return false;
}

void ctrlComboBox::Msg_ListSelectItem(const unsigned int ctrl_id, const int selection)
{
    // Liste wieder ausblenden
    ShowList(false);

    ctrlList* list = GetCtrl<ctrlList>(0);

    // ist in der Liste überhaupt was drin?
    if(selection != selectionOnListOpen && list->GetLineCount() > 0)
    {
        // Nachricht an übergeordnetes Fenster verschicken
        parent_->Msg_ComboSelectItem(GetID(), selection);
    }
}


///////////////////////////////////////////////////////////////////////////////
/**
 *  fügt einen String zur Liste hinzu.
 *
 *  @author OLiver
 */
void ctrlComboBox::AddString(const std::string& text)
{
    GetCtrl<ctrlList>(0)->AddString(text);
    Resize_(width_, height_);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  löscht alle Items der Liste.
 *
 *  @author OLiver
 */
void ctrlComboBox::DeleteAllItems()
{
    GetCtrl<ctrlList>(0)->DeleteAllItems();
    Resize_(width_, height_);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  wählt ein Item aus
 *
 *  @author OLiver
 */
void ctrlComboBox::SetSelection(unsigned short selection)
{
    // Avoid sending the change method when this is invoked intentionally
    selectionOnListOpen = selection;
    GetCtrl<ctrlList>(0)->SetSelection(selection);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  zeichnet das Fenster.
 *
 *  @author OLiver
 */
bool ctrlComboBox::Draw_()
{
    ctrlList* liste = GetCtrl<ctrlList>(0);

    // Box
    Draw3D(GetX(), GetY(), width_, height_, tc, 2);

    // Namen des selektierten Strings in der Box anzeigen
    if(liste->GetLineCount() > 0)
        font->Draw(GetX() + 2, GetY() + height_ / 2, liste->GetSelItemText(), glArchivItem_Font::DF_VCENTER, COLOR_YELLOW, 0, width_ - 2 - height_, "");

    // Male restliche Controls per Hand, denn ein einfaches DrawControls() würde
    // auch die Liste malen, die bei Msg_PaintAfter() sowieso gemalt wird.
    ctrlImageButton* button = GetCtrl<ctrlImageButton>(1);
    if(button)
        button->Draw();

    return true;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  blendet die Liste ein oder aus.
 *
 *  @author OLiver
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
    GetCtrl<ctrlButton>(1)->SetCheck(show);

    // Region sperren für die Liste, oder freigeben
    if(show)
    {
        Rect list_region (liste->GetX(), liste->GetY(), width_, liste->GetHeight());
        parent_->LockRegion(this, list_region);
        selectionOnListOpen = GetSelection();
    }
    else
    {
        parent_->FreeRegion(this);
    }

    LOADER.GetSoundN("sound", 113)->Play(255, false);
}
