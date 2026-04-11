// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
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
    AddList(0, DrawPoint(0, size.y), Extent(size.x, 4), tc, font)->SetVisible(false);

    if(!readonly)
        AddImageButton(1, DrawPoint(size.x - size.y, 0), Extent(size.y, size.y), tc, LOADER.GetImageN("io", 34));

    Resize(size);
}

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

void ctrlComboBox::AddItem(const std::string& text)
{
    GetCtrl<ctrlList>(0)->AddItem(text);
    Resize(GetSize());
}

void ctrlComboBox::DeleteAllItems()
{
    GetCtrl<ctrlList>(0)->DeleteAllItems();
    Resize(GetSize());
}

void ctrlComboBox::SetSelection(unsigned selection)
{
    // Avoid sending the change method when this is invoked intentionally
    suppressSelectEvent = true;
    GetCtrl<ctrlList>(0)->SetSelection(selection);
    suppressSelectEvent = false;
}

void ctrlComboBox::Draw_()
{
    auto* liste = GetCtrl<ctrlList>(0);

    Draw3D(Rect(GetDrawPos(), GetSize()), tc, false);

    // Show selected item in the box
    if(liste->GetNumLines() > 0)
    {
        font->Draw(GetDrawPos() + DrawPoint(2, GetSize().y / 2), liste->GetSelItemText(), FontStyle::VCENTER,
                   COLOR_YELLOW, GetSize().x - 2 - GetSize().y, "");
    }

    // Draw button manually as we can't use DrawControls which would draw the list we do in Msg_PaintAfter
    auto* button = GetCtrl<ctrlButton>(1);
    if(button)
        button->Draw();
}

void ctrlComboBox::Msg_PaintAfter()
{
    // Draw list now so it is on top of everything
    GetCtrl<ctrlList>(0)->Draw();
    Window::Msg_PaintAfter();
}

void ctrlComboBox::ShowList(bool show)
{
    auto* list = GetCtrl<ctrlList>(0);
    if(list->IsVisible() == show)
        return;

    // list field
    list->SetVisible(show);
    // Arrow button
    GetCtrl<ctrlButton>(1)->SetChecked(show);

    // Lock/unlock region of extended list
    if(show)
        GetParent()->LockRegion(this, list->GetDrawRect());
    else
        GetParent()->FreeRegion(this);

    LOADER.GetSoundN("sound", 113)->Play(255, false);
}
