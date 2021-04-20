// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "iwTransport.h"
#include "DrawPoint.h"
#include "GamePlayer.h"
#include "Loader.h"
#include "WindowManager.h"
#include "controls/ctrlImageButton.h"
#include "controls/ctrlOptionGroup.h"
#include "iwHelp.h"
#include "network/GameClient.h"
#include "world/GameWorldViewer.h"
#include "gameData/GoodConsts.h"
#include "gameData/const_gui_ids.h"

iwTransport::iwTransport(const GameWorldViewer& gwv, GameCommandFactory& gcFactory)
    : TransmitSettingsIgwAdapter(CGI_TRANSPORT, IngameWindow::posLastOrCenter, Extent(166, 333), _("Transport"),
                                 LOADER.GetImageN("io", 5)),
      gwv(gwv), gcFactory(gcFactory)
{
    AddImageButton(0, DrawPoint(18, 285), Extent(30, 30), TextureColor::Grey, LOADER.GetImageN("io", 225), _("Help"));

    // Standard
    AddImageButton(1, DrawPoint(60, 285), Extent(48, 30), TextureColor::Grey, LOADER.GetImageN("io", 191),
                   _("Default"));
    // ganz hoch
    AddImageButton(2, DrawPoint(118, 235), Extent(30, 20), TextureColor::Grey, LOADER.GetImageN("io", 215), _("Top"));
    // hoch
    AddImageButton(3, DrawPoint(118, 255), Extent(30, 20), TextureColor::Grey, LOADER.GetImageN("io", 33), _("Up"));
    // runter
    AddImageButton(4, DrawPoint(118, 275), Extent(30, 20), TextureColor::Grey, LOADER.GetImageN("io", 34), _("Down"));
    // ganz runter
    AddImageButton(5, DrawPoint(118, 295), Extent(30, 20), TextureColor::Grey, LOADER.GetImageN("io", 216),
                   _("Bottom"));

    // Buttons der einzelnen Waren anlegen
    ctrlOptionGroup* group = AddOptionGroup(6, GroupSelectType::Illuminate);

    auto getGoodTex = [](GoodType good) { return LOADER.GetMapTexN(WARES_TEX_MAP_OFFSET + rttr::enum_cast(good)); };
    buttonData = {{{getGoodTex(GoodType::Coins), WARE_NAMES[GoodType::Coins]},
                   {LOADER.GetTextureN("io", 111), gettext_noop("Weapons")},
                   {getGoodTex(GoodType::Beer), WARE_NAMES[GoodType::Beer]},
                   {getGoodTex(GoodType::Iron), WARE_NAMES[GoodType::Iron]},
                   {getGoodTex(GoodType::Gold), WARE_NAMES[GoodType::Gold]},
                   {getGoodTex(GoodType::IronOre), WARE_NAMES[GoodType::IronOre]},
                   {getGoodTex(GoodType::Coal), WARE_NAMES[GoodType::Coal]},
                   {getGoodTex(GoodType::Boards), WARE_NAMES[GoodType::Boards]},
                   {getGoodTex(GoodType::Stones), WARE_NAMES[GoodType::Stones]},
                   {getGoodTex(GoodType::Wood), WARE_NAMES[GoodType::Wood]},
                   {getGoodTex(GoodType::Water), WARE_NAMES[GoodType::Water]},
                   {LOADER.GetTextureN("io", 80), gettext_noop("Food")},
                   {getGoodTex(GoodType::Hammer), gettext_noop("Tools")},
                   {getGoodTex(GoodType::Boat), WARE_NAMES[GoodType::Boat]}}};
    // Positionen der einzelnen Buttons
    const std::array<DrawPoint, numButtons> BUTTON_POS = {{{20, 25},
                                                           {52, 42},
                                                           {84, 59},
                                                           {116, 76},
                                                           {84, 93},
                                                           {52, 110},
                                                           {20, 127},
                                                           {52, 144},
                                                           {84, 161},
                                                           {116, 178},
                                                           {84, 195},
                                                           {52, 212},
                                                           {20, 229},
                                                           {52, 246}}};

    // Get current transport order
    pendingOrder = GAMECLIENT.visual_settings.transport_order;

    // Einstellungen festlegen
    for(unsigned char i = 0; i < buttonData.size(); ++i)
    {
        group->AddImageButton(i, BUTTON_POS[i], Extent(30, 30), TextureColor::Grey, buttonData[pendingOrder[i]].sprite,
                              _(buttonData[pendingOrder[i]].tooltip));
    }
    group->SetSelection(0);
}

void iwTransport::TransmitSettings()
{
    if(GAMECLIENT.IsReplayModeOn())
        return;
    if(settings_changed)
    {
        // Daten übertragen
        if(gcFactory.ChangeTransport(pendingOrder))
        {
            GAMECLIENT.visual_settings.transport_order = pendingOrder;
            settings_changed = false;
        }
    }
}

void iwTransport::Msg_ButtonClick(const unsigned ctrl_id)
{
    if(GAMECLIENT.IsReplayModeOn())
        return;
    switch(ctrl_id)
    {
        case 0:
        {
            WINDOWMANAGER.ReplaceWindow(
              std::make_unique<iwHelp>(_("The transport priority of a type of merchandise can "
                                         "be determined here.The higher the priority of an item "
                                         "in the list, the quicker it is transported by helpers.")));
        }
        break;
        case 1: // Standardbelegung
        {
            auto* group = GetCtrl<ctrlOptionGroup>(6);

            pendingOrder = GAMECLIENT.default_settings.transport_order;

            for(unsigned char i = 0; i < buttonData.size(); ++i)
            {
                const auto& data = buttonData[i];
                group->GetCtrl<ctrlImageButton>(i)->SetImage(data.sprite);
                group->GetCtrl<ctrlImageButton>(i)->SetTooltip(_(data.tooltip));
            }

            settings_changed = true;
        }
        break;
        case 2: // ganz hoch
        {
            auto* group = GetCtrl<ctrlOptionGroup>(6);

            // Wenn wir schon ganz oben sind, gehts nicht weiter höher
            while(group->GetSelection() > 0)
            {
                std::swap(pendingOrder[group->GetSelection()], pendingOrder[group->GetSelection() - 1]);
                ctrlImageButton& btPrev = *group->GetCtrl<ctrlImageButton>(group->GetSelection() - 1);
                ctrlImageButton& btNext = *group->GetCtrl<ctrlImageButton>(group->GetSelection());
                btPrev.SwapImage(btNext);
                btPrev.SwapTooltip(btNext);
                group->SetSelection(group->GetSelection() - 1);
            }

            settings_changed = true;
        }
        break;
        case 3: // hoch
        {
            auto* group = GetCtrl<ctrlOptionGroup>(6);

            // Wenn wir schon ganz oben sind, gehts nicht weiter höher
            if(group->GetSelection() > 0)
            {
                std::swap(pendingOrder[group->GetSelection()], pendingOrder[group->GetSelection() - 1]);
                ctrlImageButton& btPrev = *group->GetCtrl<ctrlImageButton>(group->GetSelection() - 1);
                ctrlImageButton& btNext = *group->GetCtrl<ctrlImageButton>(group->GetSelection());
                btPrev.SwapImage(btNext);
                btPrev.SwapTooltip(btNext);
                group->SetSelection(group->GetSelection() - 1);
            }

            settings_changed = true;
        }
        break;
        case 4: // runter
        {
            auto* group = GetCtrl<ctrlOptionGroup>(6);

            // Wenn wir schon ganz unten sind, gehts nicht weiter runter
            if(group->GetSelection() < 13)
            {
                std::swap(pendingOrder[group->GetSelection()], pendingOrder[group->GetSelection() + 1]);
                ctrlImageButton& btPrev = *group->GetCtrl<ctrlImageButton>(group->GetSelection());
                ctrlImageButton& btNext = *group->GetCtrl<ctrlImageButton>(group->GetSelection() + 1);
                btPrev.SwapImage(btNext);
                btPrev.SwapTooltip(btNext);
                group->SetSelection(group->GetSelection() + 1);
            }

            settings_changed = true;
        }
        break;
        case 5: // ganz runter
        {
            auto* group = GetCtrl<ctrlOptionGroup>(6);

            // Wenn wir schon ganz unten sind, gehts nicht weiter runter
            while(group->GetSelection() < 13)
            {
                std::swap(pendingOrder[group->GetSelection()], pendingOrder[group->GetSelection() + 1]);
                ctrlImageButton& btPrev = *group->GetCtrl<ctrlImageButton>(group->GetSelection());
                ctrlImageButton& btNext = *group->GetCtrl<ctrlImageButton>(group->GetSelection() + 1);
                btPrev.SwapImage(btNext);
                btPrev.SwapTooltip(btNext);
                group->SetSelection(group->GetSelection() + 1);
            }

            settings_changed = true;
        }
        break;
    }
}

void iwTransport::UpdateSettings()
{
    if(GAMECLIENT.IsReplayModeOn())
    {
        gwv.GetPlayer().FillVisualSettings(GAMECLIENT.visual_settings);
        pendingOrder = GAMECLIENT.visual_settings.transport_order;
    }
    auto* group = GetCtrl<ctrlOptionGroup>(6);

    // Einstellungen festlegen
    for(unsigned char i = 0; i < buttonData.size(); ++i)
    {
        const auto& data = buttonData[pendingOrder[i]];
        group->GetCtrl<ctrlImageButton>(i)->SetImage(data.sprite);
        group->GetCtrl<ctrlImageButton>(i)->SetTooltip(_(data.tooltip));
    }
}
