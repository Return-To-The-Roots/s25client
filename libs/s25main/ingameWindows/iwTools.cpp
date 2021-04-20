// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
#include "gameData/GoodConsts.h"
#include "gameData/ToolConsts.h"
#include "gameData/const_gui_ids.h"
#include "s25util/colors.h"

iwTools::iwTools(const GameWorldViewer& gwv, GameCommandFactory& gcFactory)
    : TransmitSettingsIgwAdapter(
      CGI_TOOLS, IngameWindow::posLastOrCenter,
      Extent(166 + (gwv.GetWorld().GetGGS().isEnabled(AddonId::TOOL_ORDERING) ? 46 : 0), 432), _("Tools"),
      LOADER.GetImageN("io", 5)),
      gwv(gwv), gcFactory(gcFactory), ordersChanged(false), shouldUpdateTexts(false),
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
              200 + i, DrawPoint(151, 31 + i * 28), Extent(22, 18), TextureColor::Grey, "", NormalFont, COLOR_YELLOW));
            txt->ResizeForMaxChars(2);
            const auto txtSize = txt->GetSize();
            ctrlButton* bt = AddImageButton(100 + i * 2, txt->GetPos() + DrawPoint(txtSize.x + 1, -4), btSize,
                                            TextureColor::Grey, LOADER.GetImageN("io", 33), "+1");
            AddImageButton(101 + i * 2, bt->GetPos() + DrawPoint(0, btSize.y), btSize, TextureColor::Grey,
                           LOADER.GetImageN("io", 34), "-1");
        }
        pendingOrderChanges.fill(0);
        UpdateTexts();
    }

    // Info
    AddImageButton(12, DrawPoint(18, 384), Extent(30, 32), TextureColor::Grey, LOADER.GetImageN("io", 225), _("Help"));
    if(settings.isEnabled(AddonId::TOOL_ORDERING))
        AddImageButton(15, DrawPoint(130, 384), Extent(30, 32), TextureColor::Grey, LOADER.GetImageN("io", 216),
                       _("Zero all production"));
    // Standard
    AddImageButton(13, DrawPoint(118 + (settings.isEnabled(AddonId::TOOL_ORDERING) ? 46 : 0), 384), Extent(30, 32),
                   TextureColor::Grey, LOADER.GetImageN("io", 191), _("Default"));

    // Einstellungen festlegen
    UpdateSettings();

    toolSubscription =
      gwv.GetWorld().GetNotifications().subscribe<ToolNote>([this](auto) { this->shouldUpdateTexts = true; });
}

void iwTools::AddToolSettingSlider(unsigned id, GoodType ware)
{
    ctrlProgress* el =
      AddProgress(id, DrawPoint(17, 25 + id * 28), Extent(132, 26), TextureColor::Grey, 140 + id * 2 + 1, 140 + id * 2,
                  10, _(WARE_NAMES[ware]), Extent(4, 4), 0, _("Less often"), _("More often"));
    if(isReplay)
        el->ActivateControls(false);
}

void iwTools::TransmitSettings()
{
    if(isReplay)
        return;
    // Wurden Einstellungen verändert?
    settings_changed |= ordersChanged;
    if(settings_changed)
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
                UpdateSettings(GAMECLIENT.default_settings.tools_settings);
                settings_changed = true;
                break;
            case 15: // Zero all
                ToolSettings zero;
                zero.fill(0);
                UpdateSettings(zero);
                settings_changed = true;
                break;
        }
}

void iwTools::Msg_ProgressChange(const unsigned /*ctrl_id*/, const unsigned short /*position*/)
{
    // Einstellungen wurden geändert
    settings_changed = true;
}

void iwTools::UpdateSettings(const ToolSettings& tool_settings)
{
    if(isReplay)
        GAMECLIENT.ResetVisualSettings();
    for(unsigned i = 0; i < NUM_TOOLS; ++i)
        GetCtrl<ctrlProgress>(i)->SetPosition(tool_settings[i]);
}

void iwTools::UpdateSettings()
{
    UpdateSettings(GAMECLIENT.visual_settings.tools_settings);
}
