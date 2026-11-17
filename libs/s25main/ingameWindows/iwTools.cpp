// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
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
      Extent(166 + (gwv.getWorld().getGGS().isEnabled(AddonId::TOOL_ORDERING) ? 46 : 0), 432), _("Tools"),
      LOADER.getImageN("io", 5)),
      gwv(gwv), gcFactory(gcFactory), ordersChanged(false), shouldUpdateTexts(false),
      isReplay(GAMECLIENT.isReplayModeOn())
{
    // Einzelne Balken
    for(const auto tool : helpers::enumRange<Tool>())
        addToolSettingSlider(rttr::enumCast(tool), TOOL_TO_GOOD[tool]);

    const GlobalGameSettings& settings = gwv.getWorld().getGGS();
    if(settings.isEnabled(AddonId::TOOL_ORDERING))
    {
        // qx:tools
        for(const auto tool : helpers::enumRange<Tool>())
        {
            const auto toolIdx = rttr::enumCast(tool);
            constexpr Extent btSize(20, 13);
            auto* txt = static_cast<ctrlTextDeepening*>(
              addTextDeepening(200 + toolIdx, DrawPoint(151, 31 + toolIdx * 28), Extent(22, 18), TextureColor::Grey, "",
                               NormalFont, COLOR_YELLOW));
            txt->resizeForMaxChars(2);
            const auto txtSize = txt->getSize();
            ctrlButton* bt = addImageButton(100 + toolIdx * 2, txt->getPos() + DrawPoint(txtSize.x + 1, -4), btSize,
                                            TextureColor::Grey, LOADER.getImageN("io", 33), "+1");
            addImageButton(101 + toolIdx * 2, bt->getPos() + DrawPoint(0, btSize.y), btSize, TextureColor::Grey,
                           LOADER.getImageN("io", 34), "-1");
        }
        std::fill(pendingOrderChanges.begin(), pendingOrderChanges.end(), 0);
        updateTexts();
    }

    // Info
    addImageButton(12, DrawPoint(18, 384), Extent(30, 32), TextureColor::Grey, LOADER.getImageN("io", 225), _("Help"));
    if(settings.isEnabled(AddonId::TOOL_ORDERING))
        addImageButton(15, DrawPoint(130, 384), Extent(30, 32), TextureColor::Grey, LOADER.getImageN("io", 216),
                       _("Zero all production"));
    // Standard
    addImageButton(13, DrawPoint(118 + (settings.isEnabled(AddonId::TOOL_ORDERING) ? 46 : 0), 384), Extent(30, 32),
                   TextureColor::Grey, LOADER.getImageN("io", 191), _("Default"));

    // Einstellungen festlegen
    updateSettings();

    toolSubscription =
      gwv.getWorld().getNotifications().subscribe<ToolNote>([this](auto) noexcept { this->shouldUpdateTexts = true; });
}

void iwTools::addToolSettingSlider(unsigned id, GoodType ware)
{
    ctrlProgress* el =
      addProgress(id, DrawPoint(17, 25 + id * 28), Extent(132, 26), TextureColor::Grey, 140 + id * 2 + 1, 140 + id * 2,
                  10, _(WARE_NAMES[ware]), Extent(4, 4), 0, _("Less often"), _("More often"));
    if(isReplay)
        el->activateControls(false);
}

void iwTools::transmitSettings()
{
    if(isReplay)
        return;
    // Wurden Einstellungen verändert?
    settings_changed |= ordersChanged;
    if(settings_changed)
    {
        // Einstellungen speichern
        ToolSettings newSettings;
        for(const auto tool : helpers::enumRange<Tool>())
            newSettings[tool] = static_cast<uint8_t>(getCtrl<ctrlProgress>(rttr::enumCast(tool))->getPosition());

        int8_t* orderDelta = nullptr;
        if(ordersChanged)
        {
            orderDelta = pendingOrderChanges.data();
            const GamePlayer& localPlayer = gwv.getPlayer();
            for(const auto tool : helpers::enumRange<Tool>())
            {
                auto curOrder = static_cast<int>(localPlayer.getToolsOrderedVisual(tool));
                pendingOrderChanges[tool] = helpers::clamp<int>(pendingOrderChanges[tool], -curOrder, 100 - curOrder);
            }
        }

        if(gcFactory.ChangeTools(newSettings, orderDelta))
        {
            GAMECLIENT.visual_settings.tools_settings = newSettings;
            if(ordersChanged)
            {
                const GamePlayer& localPlayer = gwv.getPlayer();
                for(const auto tool : helpers::enumRange<Tool>())
                    localPlayer.changeToolOrderVisual(tool, pendingOrderChanges[tool]);
                std::fill(pendingOrderChanges.begin(), pendingOrderChanges.end(), 0);
            }
            settings_changed = false;
            ordersChanged = false;
        }
    }
}

void iwTools::updateTexts()
{
    if(gwv.getWorld().getGGS().isEnabled(AddonId::TOOL_ORDERING))
    {
        const GamePlayer& localPlayer = gwv.getPlayer();
        for(const auto tool : helpers::enumRange<Tool>())
        {
            auto* field = getCtrl<ctrlBaseText>(200 + rttr::enumCast(tool));
            int curOrders = isReplay ? localPlayer.getToolsOrdered(tool) :
                                       localPlayer.getToolsOrderedVisual(tool) + pendingOrderChanges[tool];
            field->setText(helpers::toString(curOrders));
        }
    }
}

void iwTools::msgPaintBefore()
{
    IngameWindow::msgPaintBefore();

    if(shouldUpdateTexts)
    {
        updateTexts();
        shouldUpdateTexts = false;
    }
}

void iwTools::msgButtonClick(const unsigned ctrl_id)
{
    if(isReplay)
        return;
    // qx:tools
    if(ctrl_id >= 100 && ctrl_id < (100 + 2 * helpers::NumEnumValues_v<Tool>))
    {
        const auto tool = static_cast<Tool>((ctrl_id - 100) / 2);
        int curOrders = gwv.getPlayer().getToolsOrderedVisual(tool) + pendingOrderChanges[tool];

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
        getCtrl<ctrlBaseText>(200 + rttr::enumCast(tool))->setText(helpers::toString(curOrders));
    } else
        switch(ctrl_id)
        {
            default: return;
            case 12:
                WINDOWMANAGER.replaceWindow(
                  std::make_unique<iwHelp>(_("These settings control the tool production of your metalworks. "
                                             "The higher the value, the more likely this tool is to be produced.")));
                break;
            case 13: // Standard
                updateSettings(GAMECLIENT.default_settings.tools_settings);
                settings_changed = true;
                break;
            case 15: // Zero all
                ToolSettings zero{};
                updateSettings(zero);
                settings_changed = true;
                break;
        }
}

void iwTools::msgProgressChange(const unsigned /*ctrl_id*/, const unsigned short /*position*/)
{
    // Einstellungen wurden geändert
    settings_changed = true;
}

void iwTools::updateSettings(const ToolSettings& tool_settings)
{
    if(isReplay)
        GAMECLIENT.resetVisualSettings();
    for(const auto tool : helpers::enumRange<Tool>())
        getCtrl<ctrlProgress>(rttr::enumCast(tool))->setPosition(tool_settings[tool]);
}

void iwTools::updateSettings()
{
    updateSettings(GAMECLIENT.visual_settings.tools_settings);
}
