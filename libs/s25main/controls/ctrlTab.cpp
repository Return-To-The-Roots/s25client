// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ctrlTab.h"
#include "Loader.h"
#include "ctrlButton.h"
#include "ctrlGroup.h"
#include "ogl/glArchivItem_Bitmap.h"
class MouseCoords;

ctrlTab::ctrlTab(Window* parent, unsigned id, const DrawPoint& pos, unsigned short width)
    : Window(parent, id, pos, Extent(width, 45)), tab_count(0), tab_selection(0)
{}

void ctrlTab::Msg_ButtonClick(const unsigned ctrl_id)
{
    SetSelection(ctrl_id, true);
}

bool ctrlTab::Msg_LeftDown(const MouseCoords& mc)
{
    return RelayMouseMessage(&Window::Msg_LeftDown, mc);
}

bool ctrlTab::Msg_LeftUp(const MouseCoords& mc)
{
    return RelayMouseMessage(&Window::Msg_LeftUp, mc);
}

bool ctrlTab::Msg_WheelUp(const MouseCoords& mc)
{
    return RelayMouseMessage(&Window::Msg_WheelUp, mc);
}

bool ctrlTab::Msg_WheelDown(const MouseCoords& mc)
{
    return RelayMouseMessage(&Window::Msg_WheelDown, mc);
}

bool ctrlTab::Msg_MouseMove(const MouseCoords& mc)
{
    return RelayMouseMessage(&Window::Msg_MouseMove, mc);
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
    GetParent()->Msg_TabChange(GetID(), tabs[nr]);

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

void ctrlTab::Msg_Group_ButtonClick(const unsigned /*group_id*/, const unsigned ctrl_id)
{
    GetParent()->Msg_Group_ButtonClick(this->GetID(), ctrl_id);
}

void ctrlTab::Msg_Group_EditEnter(const unsigned /*group_id*/, const unsigned ctrl_id)
{
    GetParent()->Msg_Group_EditEnter(this->GetID(), ctrl_id);
}

void ctrlTab::Msg_Group_EditChange(const unsigned /*group_id*/, const unsigned ctrl_id)
{
    GetParent()->Msg_Group_EditChange(this->GetID(), ctrl_id);
}

void ctrlTab::Msg_Group_TabChange(const unsigned /*group_id*/, const unsigned ctrl_id, const unsigned short tab_id)
{
    GetParent()->Msg_Group_TabChange(this->GetID(), ctrl_id, tab_id);
}

void ctrlTab::Msg_Group_ListSelectItem(const unsigned /*group_id*/, const unsigned ctrl_id, const int selection)
{
    GetParent()->Msg_Group_ListSelectItem(this->GetID(), ctrl_id, selection);
}

void ctrlTab::Msg_Group_ComboSelectItem(const unsigned /*group_id*/, const unsigned ctrl_id, const unsigned selection)
{
    GetParent()->Msg_Group_ComboSelectItem(this->GetID(), ctrl_id, selection);
}

void ctrlTab::Msg_Group_CheckboxChange(const unsigned /*group_id*/, const unsigned ctrl_id, const bool checked)
{
    GetParent()->Msg_Group_CheckboxChange(this->GetID(), ctrl_id, checked);
}

void ctrlTab::Msg_Group_ProgressChange(const unsigned /*group_id*/, const unsigned ctrl_id,
                                       const unsigned short position)
{
    GetParent()->Msg_Group_ProgressChange(this->GetID(), ctrl_id, position);
}

void ctrlTab::Msg_Group_ScrollShow(const unsigned /*group_id*/, const unsigned ctrl_id, const bool visible)
{
    GetParent()->Msg_Group_ScrollShow(this->GetID(), ctrl_id, visible);
}

void ctrlTab::Msg_Group_OptionGroupChange(const unsigned /*group_id*/, const unsigned ctrl_id, const unsigned selection)
{
    GetParent()->Msg_Group_OptionGroupChange(this->GetID(), ctrl_id, selection);
}

void ctrlTab::Msg_Group_Timer(const unsigned /*group_id*/, const unsigned ctrl_id)
{
    GetParent()->Msg_Group_Timer(this->GetID(), ctrl_id);
}

void ctrlTab::Msg_Group_TableSelectItem(const unsigned /*group_id*/, const unsigned ctrl_id,
                                        const boost::optional<unsigned>& selection)
{
    GetParent()->Msg_Group_TableSelectItem(this->GetID(), ctrl_id, selection);
}

void ctrlTab::Msg_Group_TableRightButton(const unsigned /*group_id*/, const unsigned ctrl_id,
                                         const boost::optional<unsigned>& selection)
{
    GetParent()->Msg_Group_TableRightButton(this->GetID(), ctrl_id, selection);
}

void ctrlTab::Msg_Group_TableLeftButton(const unsigned /*group_id*/, const unsigned ctrl_id,
                                        const boost::optional<unsigned>& selection)
{
    GetParent()->Msg_Group_TableLeftButton(this->GetID(), ctrl_id, selection);
}
