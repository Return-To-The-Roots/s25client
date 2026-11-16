// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ctrlTab.h"
#include "Loader.h"
#include "ctrlButton.h"
#include "ctrlGroup.h"
#include "ogl/glArchivItem_Bitmap.h"
struct MouseCoords;

ctrlTab::ctrlTab(Window* parent, unsigned id, const DrawPoint& pos, unsigned short width)
    : Window(parent, id, pos, Extent(width, 45)), tab_count(0), tab_selection(0)
{}

void ctrlTab::msgButtonClick(const unsigned ctrl_id)
{
    SetSelection(ctrl_id, true);
}

bool ctrlTab::msgLeftDown(const MouseCoords& mc)
{
    return RelayMouseMessage(&Window::msgLeftDown, mc);
}

bool ctrlTab::msgLeftUp(const MouseCoords& mc)
{
    return RelayMouseMessage(&Window::msgLeftUp, mc);
}

bool ctrlTab::msgWheelUp(const MouseCoords& mc)
{
    return RelayMouseMessage(&Window::msgWheelUp, mc);
}

bool ctrlTab::msgWheelDown(const MouseCoords& mc)
{
    return RelayMouseMessage(&Window::msgWheelDown, mc);
}

bool ctrlTab::msgMouseMove(const MouseCoords& mc)
{
    return RelayMouseMessage(&Window::msgMouseMove, mc);
}

/**
 *  fügt eine Tab hinzu.
 */
ctrlGroup* ctrlTab::AddTab(glArchivItem_Bitmap* image, const std::string& tooltip, const unsigned id)
{
    if(tab_count < tabs.size())
    {
        if(AddImageButton(tab_count, DrawPoint(36 * tab_count, 0), Extent(36, 45), TextureColor::Red1, image, tooltip))
        {
            tabs[tab_count++] = id;
            ctrlGroup* group = AddGroup(tabs.size() + 1 + id);
            group->SetVisible(false);

            return group;
        }
    }

    return nullptr;
}

/**
 *  löscht alle Tabs.
 */
void ctrlTab::DeleteAllTabs()
{
    for(unsigned i = 0; i < tab_count; ++i)
        DeleteCtrl(i);

    tab_selection = 0;
    tab_count = 0;
}
/**
 *  aktiviert eine bestimmte Tabseite.
 */
void ctrlTab::SetSelection(unsigned short nr, bool /*notify*/)
{
    if(nr >= tab_count)
        return;

    /// Eltern informieren, dass Tab geändert wurde
    GetParent()->msgTabChange(GetID(), tabs[nr]);

    // Farbe des alten Buttons ändern
    ctrlButton* button;

    button = GetCtrl<ctrlButton>(tab_selection);
    if(button)
        button->SetTexture(TextureColor::Red1);

    // Steuerelemente auf der alten Tabseite ausblenden
    GetCtrl<ctrlGroup>(tabs[tab_selection] + tabs.size() + 1)->SetVisible(false);

    // Umwählen
    tab_selection = nr;

    // Farbe des neuen Buttons ändern
    button = GetCtrl<ctrlButton>(tab_selection);
    if(button)
        button->SetTexture(TextureColor::Green1);

    // Steuerelemente auf der neuen Tabseite einblenden
    GetCtrl<ctrlGroup>(tabs[nr] + tabs.size() + 1)->SetVisible(true);
}

/**
 *  Gibt Tab-Group zurück, über die die Steuerelemente der Tab angesprochen
 *  werden können
 */
ctrlGroup* ctrlTab::GetGroup(const unsigned tab_id)
{
    return GetCtrl<ctrlGroup>(tabs.size() + 1 + tab_id);
}

/**
 *  Zeichenmethode
 *
 *  @return @p true bei Erfolg, @p false bei Fehler
 */
void ctrlTab::Draw_()
{
    Window::Draw_();

    // TODO: What is this really?
    int headerSize = tab_count * 36;
    LOADER.GetImageN("io", 3)->DrawPart(
      Rect(GetDrawPos() + DrawPoint(headerSize, 0), Extent(GetSize().x - headerSize, 45)));

    Draw3D(Rect(GetDrawPos() + DrawPoint(0, 32), Extent(GetSize().x, 13)), TextureColor::Green1, true);

    auto* button = GetCtrl<ctrlButton>(tab_selection);
    if(button)
        button->Draw();
}

void ctrlTab::msgGroupButtonClick(const unsigned /*group_id*/, const unsigned ctrl_id)
{
    GetParent()->msgGroupButtonClick(this->GetID(), ctrl_id);
}

void ctrlTab::msgGroupEditEnter(const unsigned /*group_id*/, const unsigned ctrl_id)
{
    GetParent()->msgGroupEditEnter(this->GetID(), ctrl_id);
}

void ctrlTab::msgGroupEditChange(const unsigned /*group_id*/, const unsigned ctrl_id)
{
    GetParent()->msgGroupEditChange(this->GetID(), ctrl_id);
}

void ctrlTab::msgGroupTabChange(const unsigned /*group_id*/, const unsigned ctrl_id, const unsigned short tab_id)
{
    GetParent()->msgGroupTabChange(this->GetID(), ctrl_id, tab_id);
}

void ctrlTab::msgGroupListSelectItem(const unsigned /*group_id*/, const unsigned ctrl_id, const int selection)
{
    GetParent()->msgGroupListSelectItem(this->GetID(), ctrl_id, selection);
}

void ctrlTab::msgGroupComboSelectItem(const unsigned /*group_id*/, const unsigned ctrl_id, const unsigned selection)
{
    GetParent()->msgGroupComboSelectItem(this->GetID(), ctrl_id, selection);
}

void ctrlTab::msgGroupCheckboxChange(const unsigned /*group_id*/, const unsigned ctrl_id, const bool checked)
{
    GetParent()->msgGroupCheckboxChange(this->GetID(), ctrl_id, checked);
}

void ctrlTab::msgGroupProgressChange(const unsigned /*group_id*/, const unsigned ctrl_id, const unsigned short position)
{
    GetParent()->msgGroupProgressChange(this->GetID(), ctrl_id, position);
}

void ctrlTab::msgGroupScrollShow(const unsigned /*group_id*/, const unsigned ctrl_id, const bool visible)
{
    GetParent()->msgGroupScrollShow(this->GetID(), ctrl_id, visible);
}

void ctrlTab::msgGroupOptionGroupChange(const unsigned /*group_id*/, const unsigned ctrl_id, const unsigned selection)
{
    GetParent()->msgGroupOptionGroupChange(this->GetID(), ctrl_id, selection);
}

void ctrlTab::msgGroupTimer(const unsigned /*group_id*/, const unsigned ctrl_id)
{
    GetParent()->msgGroupTimer(this->GetID(), ctrl_id);
}

void ctrlTab::msgGroupTableSelectItem(const unsigned /*group_id*/, const unsigned ctrl_id,
                                      const std::optional<unsigned>& selection)
{
    GetParent()->msgGroupTableSelectItem(this->GetID(), ctrl_id, selection);
}

void ctrlTab::msgGroupTableRightButton(const unsigned /*group_id*/, const unsigned ctrl_id,
                                       const std::optional<unsigned>& selection)
{
    GetParent()->msgGroupTableRightButton(this->GetID(), ctrl_id, selection);
}

void ctrlTab::msgGroupTableLeftButton(const unsigned /*group_id*/, const unsigned ctrl_id,
                                      const std::optional<unsigned>& selection)
{
    GetParent()->msgGroupTableLeftButton(this->GetID(), ctrl_id, selection);
}
