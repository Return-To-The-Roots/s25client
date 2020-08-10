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
#include "gameData/const_gui_ids.h"

const std::array<std::string, 14> TOOLTIPS = {
  gettext_noop("Coins"),    gettext_noop("Weapons"), gettext_noop("Beer"),   gettext_noop("Iron"),   gettext_noop("Gold"),
  gettext_noop("Iron ore"), gettext_noop("Coal"),    gettext_noop("Boards"), gettext_noop("Stones"), gettext_noop("Wood"),
  gettext_noop("Water"),    gettext_noop("Food"),    gettext_noop("Tools"),  gettext_noop("Boats")};

iwTransport::iwTransport(const GameWorldViewer& gwv, GameCommandFactory& gcFactory)
    : IngameWindow(CGI_TRANSPORT, IngameWindow::posLastOrCenter, Extent(166, 333), _("Transport"), LOADER.GetImageN("io", 5)), gwv(gwv),
      gcFactory(gcFactory), settings_changed(false)
{
    AddImageButton(0, DrawPoint(18, 285), Extent(30, 30), TC_GREY, LOADER.GetImageN("io", 225), _("Help"));

    // Standard
    AddImageButton(1, DrawPoint(60, 285), Extent(48, 30), TC_GREY, LOADER.GetImageN("io", 191), _("Default"));
    // ganz hoch
    AddImageButton(2, DrawPoint(118, 235), Extent(30, 20), TC_GREY, LOADER.GetImageN("io", 215), _("Top"));
    // hoch
    AddImageButton(3, DrawPoint(118, 255), Extent(30, 20), TC_GREY, LOADER.GetImageN("io", 33), _("Up"));
    // runter
    AddImageButton(4, DrawPoint(118, 275), Extent(30, 20), TC_GREY, LOADER.GetImageN("io", 34), _("Down"));
    // ganz runter
    AddImageButton(5, DrawPoint(118, 295), Extent(30, 20), TC_GREY, LOADER.GetImageN("io", 216), _("Bottom"));

    // Buttons der einzelnen Waren anlegen
    ctrlOptionGroup* group = AddOptionGroup(6, ctrlOptionGroup::ILLUMINATE);

    // Zeiger auf die Bilder für die einzelnen Waren in der Transportschlange
    TRANSPORT_SPRITES[0] = LOADER.GetMapTexN(2250 + GD_COINS);
    TRANSPORT_SPRITES[1] = LOADER.GetTextureN("io", 111);
    TRANSPORT_SPRITES[2] = LOADER.GetMapTexN(2250 + GD_BEER);
    TRANSPORT_SPRITES[3] = LOADER.GetMapTexN(2250 + GD_IRON);
    TRANSPORT_SPRITES[4] = LOADER.GetMapTexN(2250 + GD_GOLD);
    TRANSPORT_SPRITES[5] = LOADER.GetMapTexN(2250 + GD_IRONORE);
    TRANSPORT_SPRITES[6] = LOADER.GetMapTexN(2250 + GD_COAL);
    TRANSPORT_SPRITES[7] = LOADER.GetMapTexN(2250 + GD_BOARDS);
    TRANSPORT_SPRITES[8] = LOADER.GetMapTexN(2250 + GD_STONES);
    TRANSPORT_SPRITES[9] = LOADER.GetMapTexN(2250 + GD_WOOD);
    TRANSPORT_SPRITES[10] = LOADER.GetMapTexN(2250 + GD_WATER);
    TRANSPORT_SPRITES[11] = LOADER.GetTextureN("io", 80);
    TRANSPORT_SPRITES[12] = LOADER.GetMapTexN(2250 + GD_HAMMER);
    TRANSPORT_SPRITES[13] = LOADER.GetMapTexN(2250 + GD_BOAT);

    // Positionen der einzelnen Buttons
    const std::array<DrawPoint, 14> BUTTON_POS = {{{20, 25},
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

    // Einstellungen festlegen
    for(unsigned char i = 0; i < 14; ++i)
        group->AddImageButton(i, BUTTON_POS[i], Extent(30, 30), TC_GREY, TRANSPORT_SPRITES[GAMECLIENT.visual_settings.transport_order[i]],
                              _(TOOLTIPS[GAMECLIENT.visual_settings.transport_order[i]]));

    // Netzwerk-Übertragungs-Timer
    AddTimer(7, 2000);
}

iwTransport::~iwTransport()
{
    TransmitSettings();
}

void iwTransport::TransmitSettings()
{
    if(GAMECLIENT.IsReplayModeOn())
        return;
    if(settings_changed)
    {
        // Daten übertragen
        gcFactory.ChangeTransport(GAMECLIENT.visual_settings.transport_order);

        settings_changed = false;
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
            WINDOWMANAGER.ReplaceWindow(std::make_unique<iwHelp>(_("The transport priority of a type of merchandise can "
                                                                   "be determined here.The higher the priority of an item "
                                                                   "in the list, the quicker it is transported by helpers.")));
        }
        break;
        case 1: // Standardbelegung
        {
            auto* group = GetCtrl<ctrlOptionGroup>(6);

            GAMECLIENT.visual_settings.transport_order = GAMECLIENT.default_settings.transport_order;

            //// Tooltips in der der Standardbelegung
            // memcpy(tooltip_indices,STD_TOOLTIP_INDICES,14*sizeof(unsigned short));

            for(unsigned char i = 0; i < 14; ++i)
            {
                group->GetCtrl<ctrlImageButton>(i)->SetImage(TRANSPORT_SPRITES[i]);
                group->GetCtrl<ctrlImageButton>(i)->SetTooltip(_(TOOLTIPS[i]));
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
                std::swap(GAMECLIENT.visual_settings.transport_order[group->GetSelection()],
                          GAMECLIENT.visual_settings.transport_order[group->GetSelection() - 1]);
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
                std::swap(GAMECLIENT.visual_settings.transport_order[group->GetSelection()],
                          GAMECLIENT.visual_settings.transport_order[group->GetSelection() - 1]);
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
                std::swap(GAMECLIENT.visual_settings.transport_order[group->GetSelection()],
                          GAMECLIENT.visual_settings.transport_order[group->GetSelection() + 1]);
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
                std::swap(GAMECLIENT.visual_settings.transport_order[group->GetSelection()],
                          GAMECLIENT.visual_settings.transport_order[group->GetSelection() + 1]);
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

void iwTransport::Msg_Timer(const unsigned /*ctrl_id*/)
{
    if(GAMECLIENT.IsReplayModeOn())
        // Im Replay aktualisieren wir die Werte
        UpdateSettings();
    else
        // Im normalen Spielmodus schicken wir den ganzen Spaß ab
        TransmitSettings();
}

void iwTransport::UpdateSettings()
{
    if(GAMECLIENT.IsReplayModeOn())
        gwv.GetPlayer().FillVisualSettings(GAMECLIENT.visual_settings);
    auto* group = GetCtrl<ctrlOptionGroup>(6);

    // Einstellungen festlegen
    for(unsigned char i = 0; i < 14; ++i)
    {
        group->GetCtrl<ctrlImageButton>(i)->SetImage(TRANSPORT_SPRITES[GAMECLIENT.visual_settings.transport_order[i]]);
        group->GetCtrl<ctrlImageButton>(i)->SetTooltip(_(TOOLTIPS[GAMECLIENT.visual_settings.transport_order[i]]));
    }
}
