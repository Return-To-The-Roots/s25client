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

#include "iwTools.h"
#include "GamePlayer.h"
#include "GlobalGameSettings.h"
#include "Loader.h"
#include "WindowManager.h"
#include "addons/const_addons.h"
#include "controls/ctrlBaseText.h"
#include "controls/ctrlButton.h"
#include "controls/ctrlProgress.h"
#include "controls/ctrlTextDeepening.h"
#include "helpers/mathFuncs.h"
#include "helpers/toString.h"
#include "iwHelp.h"
#include "network/GameClient.h"
#include "notifications/NotificationManager.h"
#include "notifications/ToolNote.h"
#include "world/GameWorldBase.h"
#include "world/GameWorldViewer.h"
#include "gameData/ToolConsts.h"
#include "gameData/const_gui_ids.h"
#include "s25util/colors.h"

iwTools::iwTools(const GameWorldViewer& gwv, GameCommandFactory& gcFactory)
    : IngameWindow(CGI_TOOLS, IngameWindow::posLastOrCenter,
                   Extent(166 + (gwv.GetWorld().GetGGS().isEnabled(AddonId::TOOL_ORDERING) ? 46 : 0), 432), _("Tools"),
                   LOADER.GetImageN("io", 5)),
      gwv(gwv), gcFactory(gcFactory), settings_changed(false), ordersChanged(false), shouldUpdateTexts(false),
      isReplay(GAMECLIENT.IsReplayModeOn())
{
    // Einzelne Balken
    for(unsigned i = 0; i < NUM_TOOLS; i++)
        AddToolSettingSlider(i, TOOLS[i]);

    const GlobalGameSettings& settings = gwv.GetWorld().GetGGS();
    if(settings.isEnabled(AddonId::TOOL_ORDERING))
    {
        // qx:tools
        for(unsigned i = 0; i < NUM_TOOLS; ++i)
        {
            constexpr Extent btSize(20, 13);
            auto* txt = static_cast<ctrlTextDeepening*>(AddTextDeepening(
              200 + i, DrawPoint(151, 31 + i * 28), Extent(22, 18), TC_GREY, "", NormalFont, COLOR_YELLOW));
            txt->ResizeForMaxChars(2);
            const auto txtSize = txt->GetSize();
            ctrlButton* bt = AddImageButton(100 + i * 2, txt->GetPos() + DrawPoint(txtSize.x + 1, -4), btSize, TC_GREY,
                                            LOADER.GetImageN("io", 33), "+1");
            AddImageButton(101 + i * 2, bt->GetPos() + DrawPoint(0, btSize.y), btSize, TC_GREY,
                           LOADER.GetImageN("io", 34), "-1");
        }
        pendingOrderChanges.fill(0);
        UpdateTexts();
    }

    // Info
    AddImageButton(12, DrawPoint(18, 384), Extent(30, 32), TC_GREY, LOADER.GetImageN("io", 225), _("Help"));
    if(settings.isEnabled(AddonId::TOOL_ORDERING))
        AddImageButton(15, DrawPoint(130, 384), Extent(30, 32), TC_GREY, LOADER.GetImageN("io", 216),
                       _("Zero all production"));
    // Standard
    AddImageButton(13, DrawPoint(118 + (settings.isEnabled(AddonId::TOOL_ORDERING) ? 46 : 0), 384), Extent(30, 32),
                   TC_GREY, LOADER.GetImageN("io", 191), _("Default"));

    // Einstellungen festlegen
    UpdateSettings();

    // Netzwerk-Übertragungs-Timer
    AddTimer(14, 2000);

    toolSubscription =
      gwv.GetWorld().GetNotifications().subscribe<ToolNote>([this](auto) { this->shouldUpdateTexts = true; });
}

void iwTools::AddToolSettingSlider(unsigned id, GoodType ware)
{
    ctrlProgress* el =
      AddProgress(id, DrawPoint(17, 25 + id * 28), Extent(132, 26), TC_GREY, 140 + id * 2 + 1, 140 + id * 2, 10,
                  _(WARE_NAMES[ware]), Extent(4, 4), 0, _("Less often"), _("More often"));
    if(isReplay)
        el->ActivateControls(false);
}

iwTools::~iwTools()
{
    TransmitSettings();
}

void iwTools::TransmitSettings()
{
    if(isReplay)
        return;
    // Wurden Einstellungen verändert?
    if(settings_changed || ordersChanged)
    {
        // Einstellungen speichern
        ToolSettings newSettings;
        for(unsigned i = 0; i < NUM_TOOLS; ++i)
            newSettings[i] = static_cast<uint8_t>(GetCtrl<ctrlProgress>(i)->GetPosition());

        int8_t* orderDelta = nullptr;
        if(ordersChanged)
        {
            orderDelta = &pendingOrderChanges[0];
            const GamePlayer& localPlayer = gwv.GetPlayer();
            for(unsigned i = 0; i < NUM_TOOLS; ++i)
            {
                auto curOrder = static_cast<int>(localPlayer.GetToolsOrderedVisual(i));
                pendingOrderChanges[i] = helpers::clamp<int>(pendingOrderChanges[i], -curOrder, 100 - curOrder);
            }
        }

        if(gcFactory.ChangeTools(newSettings, orderDelta))
        {
            GAMECLIENT.visual_settings.tools_settings = newSettings;
            if(ordersChanged)
            {
                const GamePlayer& localPlayer = gwv.GetPlayer();
                for(unsigned i = 0; i < NUM_TOOLS; ++i)
                    localPlayer.ChangeToolOrderVisual(i, pendingOrderChanges[i]);
                pendingOrderChanges.fill(0);
            }
            settings_changed = false;
            ordersChanged = false;
        }
    }
}

void iwTools::UpdateTexts()
{
    if(gwv.GetWorld().GetGGS().isEnabled(AddonId::TOOL_ORDERING))
    {
        const GamePlayer& localPlayer = gwv.GetPlayer();
        for(unsigned i = 0; i < NUM_TOOLS; ++i)
        {
            auto* field = GetCtrl<ctrlBaseText>(200 + i);
            int curOrders =
              isReplay ? localPlayer.GetToolsOrdered(i) : localPlayer.GetToolsOrderedVisual(i) + pendingOrderChanges[i];
            field->SetText(helpers::toString(curOrders));
        }
    }
}

void iwTools::Msg_PaintBefore()
{
    IngameWindow::Msg_PaintBefore();

    if(shouldUpdateTexts)
    {
        UpdateTexts();
        shouldUpdateTexts = false;
    }
}

void iwTools::Msg_ButtonClick(const unsigned ctrl_id)
{
    if(isReplay)
        return;
    // qx:tools
    if(ctrl_id >= 100 && ctrl_id < (100 + 2 * NUM_TOOLS))
    {
        unsigned tool = (ctrl_id - 100) / 2;
        int curOrders = gwv.GetPlayer().GetToolsOrderedVisual(tool) + pendingOrderChanges[tool];

        if(ctrl_id & 0x1)
        {
            if(curOrders < 1)
                return;
            --pendingOrderChanges[tool];
            --curOrders;
        } else if(curOrders >= 99)
            return;
        else
        {
            ++pendingOrderChanges[tool];
            ++curOrders;
        }
        ordersChanged = true;
        GetCtrl<ctrlBaseText>(200 + tool)->SetText(helpers::toString(curOrders));
    } else
        switch(ctrl_id)
        {
            default: return;
            case 12:
                WINDOWMANAGER.ReplaceWindow(
                  std::make_unique<iwHelp>(_("These settings control the tool production of your metalworks. "
                                             "The higher the value, the more likely this tool is to be produced.")));
                break;
            case 13: // Standard
                GAMECLIENT.visual_settings.tools_settings = GAMECLIENT.default_settings.tools_settings;
                UpdateSettings();
                settings_changed = true;
                break;
            case 15: // Zero all
                std::fill(GAMECLIENT.visual_settings.tools_settings.begin(),
                          GAMECLIENT.visual_settings.tools_settings.end(), 0);
                UpdateSettings();
                settings_changed = true;
                break;
        }
}

void iwTools::Msg_ProgressChange(const unsigned /*ctrl_id*/, const unsigned short /*position*/)
{
    // Einstellungen wurden geändert
    settings_changed = true;
}

void iwTools::Msg_Timer(const unsigned /*ctrl_id*/)
{
    if(isReplay)
        // Im Replay aktualisieren wir die Werte
        UpdateSettings();
    else
        // Im normalen Spielmodus schicken wir den ganzen Spaß ab
        TransmitSettings();
}

void iwTools::UpdateSettings()
{
    if(isReplay)
    {
        const GamePlayer& localPlayer = gwv.GetPlayer();
        for(unsigned i = 0; i < NUM_TOOLS; ++i)
            GetCtrl<ctrlProgress>(i)->SetPosition(localPlayer.GetToolPriority(i));
    } else
    {
        for(unsigned i = 0; i < NUM_TOOLS; ++i)
            GetCtrl<ctrlProgress>(i)->SetPosition(GAMECLIENT.visual_settings.tools_settings[i]);
    }
}
