// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ctrlComboBox.h"
#include "CollisionDetection.h"
#include "Loader.h"
#include "ctrlButton.h"
#include "ctrlList.h"
#include "driver/MouseCoords.h"
#include "ogl/FontStyle.h"
#include "ogl/SoundEffectItem.h"
#include "ogl/glFont.h"

ctrlComboBox::ctrlComboBox(Window* parent, unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
                           const glFont* font, unsigned short max_list_height, bool readonly)
    : Window(parent, id, pos, size), tc(tc), font(font), max_list_height(max_list_height), readonly(readonly),
      suppressSelectEvent(false)
{
    ctrlList* liste = AddList(0, DrawPoint(0, size.y), Extent(size.x, 4), tc, font);

    // Liste am Anfang nicht anzeigen
    liste->SetVisible(false);

    if(!readonly)
        AddImageButton(1, DrawPoint(size.x - size.y, 0), Extent(size.y, size.y), tc, LOADER.GetImageN("io", 34));

    Resize(size);
}

/**
 *  Größe verändern
 */
void ctrlComboBox::Resize(const Extent& newSize)
{
    Window::Resize(newSize);

    auto* button = GetCtrl<ctrlButton>(1);
    if(button)
    {
        button->SetPos(DrawPoint(newSize.x - newSize.y, 0));
        button->Resize(Extent(newSize.y, newSize.y));
    }

    auto* list = GetCtrl<ctrlList>(0);

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

boost::optional<std::string> ctrlComboBox::GetSelectedText() const
{
    const boost::optional<unsigned>& selection = GetSelection();
    if(selection)
        return GetText(*selection);
    else
        return boost::none;
}

void ctrlComboBox::Msg_PaintAfter()
{
    // Liste erst jetzt malen, damit sie den Rest überdeckt
    GetCtrl<ctrlList>(0)->Draw();
    Window::Msg_PaintAfter();
}

bool ctrlComboBox::Msg_MouseMove(const MouseCoords& mc)
{
    // Für Button und Liste weiterleiten
    return RelayMouseMessage(&Window::Msg_MouseMove, mc);
}

bool ctrlComboBox::Msg_LeftDown(const MouseCoords& mc)
{
    auto* list = GetCtrl<ctrlList>(0);

    // Irgendwo anders hingeklickt --> Liste ausblenden
    if(!readonly && !IsPointInRect(mc.pos, GetFullDrawRect(list)))
    {
        // Liste wieder ausblenden
        ShowList(false);
        return false;
    }

    if(!readonly && IsPointInRect(mc.pos, GetDrawRect()))
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
    auto* list = GetCtrl<ctrlList>(0);

    // Für Button und Liste weiterleiten (und danach erst schließen)
    bool ret = RelayMouseMessage(&Window::Msg_RightDown, mc);

    // Clicked on list -> close it
    if(!readonly && IsPointInRect(mc.pos, list->GetDrawRect()))
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

    auto* list = GetCtrl<ctrlList>(0);
    if(list->IsVisible() && IsPointInRect(mc.pos, list->GetDrawRect()))
        return RelayMouseMessage(&Window::Msg_WheelUp, mc);

    if(IsPointInRect(mc.pos, GetDrawRect()))
    {
        // Don't scroll too far down
        if(list->GetSelection().value_or(0u) > 0u)
            list->SetSelection(*list->GetSelection() - 1u);
        return true;
    }

    return false;
}

bool ctrlComboBox::Msg_WheelDown(const MouseCoords& mc)
{
    if(readonly)
        return false;

    auto* list = GetCtrl<ctrlList>(0);

    if(list->IsVisible() && IsPointInRect(mc.pos, list->GetDrawRect()))
    {
        // Scrolled in opened list ->
        return RelayMouseMessage(&Window::Msg_WheelDown, mc);
    }

    if(IsPointInRect(mc.pos, GetDrawRect()))
    {
        // Will be ignored by the list if to high
        list->SetSelection(list->GetSelection() ? *list->GetSelection() + 1u : 0u);
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

void ctrlComboBox::Msg_ListSelectItem(unsigned, const int selection)
{
    // Liste wieder ausblenden
    ShowList(false);

    // ist in der Liste überhaupt was drin?
    if(selection >= 0 && !suppressSelectEvent)
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
    suppressSelectEvent = true;
    GetCtrl<ctrlList>(0)->SetSelection(selection);
    suppressSelectEvent = false;
}

/**
 *  zeichnet das Fenster.
 */
void ctrlComboBox::Draw_()
{
    auto* liste = GetCtrl<ctrlList>(0);

    // Box
    Draw3D(Rect(GetDrawPos(), GetSize()), tc, false);

    // Namen des selektierten Strings in der Box anzeigen
    if(liste->GetNumLines() > 0)
        font->Draw(GetDrawPos() + DrawPoint(2, GetSize().y / 2), liste->GetSelItemText(), FontStyle::VCENTER,
                   COLOR_YELLOW, GetSize().x - 2 - GetSize().y, "");

    // Male restliche Controls per Hand, denn ein einfaches DrawControls() würde
    // auch die Liste malen, die bei Msg_PaintAfter() sowieso gemalt wird.
    auto* button = GetCtrl<ctrlButton>(1);
    if(button)
        button->Draw();
}

/**
 *  blendet die Liste ein oder aus.
 */
void ctrlComboBox::ShowList(bool show)
{
    auto* liste = GetCtrl<ctrlList>(0);
    if(liste->IsVisible() == show)
        return;

    // Liste entsprechend
    liste->SetVisible(show);

    // Pfeilbutton entsprechend
    GetCtrl<ctrlButton>(1)->SetChecked(show);

    // Region sperren für die Liste, oder freigeben
    if(show)
        GetParent()->LockRegion(this, liste->GetDrawRect());
    else
        GetParent()->FreeRegion(this);

    LOADER.GetSoundN("sound", 113)->Play(255, false);
}
