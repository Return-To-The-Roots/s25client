// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "iwAIDebug.h"
#include "Loader.h"
#include "ai/AIEvents.h"
#include "ai/aijh/AIPlayerJH.h"
#include "ai/aijh/Jobs.h"
#include "controls/ctrlComboBox.h"
#include "controls/ctrlMultiline.h"
#include "helpers/EnumArray.h"
#include "helpers/toString.h"
#include "ogl/FontStyle.h"
#include "ogl/glFont.h"
#include "world/GameWorldView.h"
#include "gameData/BuildingConsts.h"
#include "gameData/const_gui_ids.h"
#include "s25util/colors.h"
#include <array>
#include <sstream>

namespace {
enum
{
    ID_CbPlayer,
    ID_CbOverlay,
    ID_Text
};
}

class iwAIDebug::DebugPrinter : public IDrawNodeCallback
{
    helpers::EnumArray<ITexture*, BuildingQuality> bqImgs;
    std::array<ITexture*, 2> ticks;
    glFont& font;

public:
    DebugPrinter(const AIJH::AIPlayerJH* ai, unsigned overlay) : font(*NormalFont), ai(ai), overlay(overlay)
    {
        // Cache images
        for(const auto i : helpers::enumRange<BuildingQuality>())
        {
            bqImgs[i] = LOADER.getMapTexture(49 + rttr::enumCast(i));
        }
        ticks[0] = LOADER.getTextureN("io", 40);
        ticks[1] = LOADER.getTextureN("io", 32);
        bqImgs[BuildingQuality::Nothing] = nullptr;
        bqImgs[BuildingQuality::Harbor] = ticks[0]; // Invalid marker
    }

    const AIJH::AIPlayerJH* ai;
    unsigned overlay;

    void onDraw(const MapPoint& pt, const DrawPoint& curPos) override
    {
        if(overlay == 0)
            return;
        if(overlay == 1)
        {
            auto* img = bqImgs[ai->getAINode(pt).bq];
            if(img)
                img->drawFull(curPos);
        } else if(overlay == 2)
            ticks[ai->getAINode(pt).reachable]->drawFull(curPos);
        else if(overlay == 3)
            ticks[ai->getAINode(pt).farmed]->drawFull(curPos);
        else if(overlay < 13)
            font.draw(curPos, helpers::toString(ai->getResMapValue(pt, AIResource(overlay - 4))), FontStyle{},
                      COLOR_YELLOW);
    }
};

iwAIDebug::iwAIDebug(GameWorldView& gwv, const std::vector<const AIPlayer*>& ais)
    : IngameWindow(CGI_AI_DEBUG, IngameWindow::posLastOrCenter, Extent(280, 515), _("AI Debug"),
                   LOADER.getImageN("resource", 41)),
      gwv(gwv), text(nullptr), printer(nullptr)
{
    for(const AIPlayer* ai : ais)
    {
        const auto* aijh = dynamic_cast<const AIJH::AIPlayerJH*>(ai);
        if(aijh)
            ais_.push_back(aijh);
    }
    // Wenn keine KI-Spieler, schließen
    if(ais_.empty())
    {
        close();
        return;
    }

    ctrlComboBox* players =
      addComboBox(ID_CbPlayer, DrawPoint(15, 30), Extent(250, 20), TextureColor::Grey, NormalFont, 100);
    for(const AIJH::AIPlayerJH* ai : ais_)
    {
        players->addItem(ai->getPlayerName());
    }

    ctrlComboBox* overlays =
      addComboBox(ID_CbOverlay, DrawPoint(15, 60), Extent(250, 20), TextureColor::Grey, NormalFont, 100);
    overlays->addItem("None");
    overlays->addItem("BuildingQuality");
    overlays->addItem("Reachability");
    overlays->addItem("Farmed");
    overlays->addItem("Gold");
    overlays->addItem("Ironore");
    overlays->addItem("Coal");
    overlays->addItem("Granite");
    overlays->addItem("Fish");
    overlays->addItem("Wood");
    overlays->addItem("Stones");
    overlays->addItem("Plantspace");
    overlays->addItem("Borderland");

    // Show 7 lines of text and 1 empty line
    text = addMultiline(ID_Text, DrawPoint(15, 120), Extent(250, 8 * NormalFont->getHeight()), TextureColor::Grey,
                        NormalFont, FontStyle::NO_OUTLINE);

    setIwSize(Extent(getIwSize().x, text->getPos().y + text->getSize().y));

    players->setSelection(0);
    overlays->setSelection(0);
    printer = new DebugPrinter(ais_[0], 0);
    gwv.addDrawNodeCallback(printer);
}

iwAIDebug::~iwAIDebug()
{
    if(printer)
    {
        gwv.removeDrawNodeCallback(printer);
        delete printer;
    }
}

void iwAIDebug::msgComboSelectItem(const unsigned ctrl_id, const unsigned selection)
{
    switch(ctrl_id)
    {
        case ID_CbPlayer: printer->ai = ais_[selection]; break;
        case ID_CbOverlay: printer->overlay = selection; break;
    }
}

void iwAIDebug::msgPaintBefore()
{
    IngameWindow::msgPaintBefore();
    std::stringstream ss;

    const AIJH::AIJob* currentJob = printer->ai->getCurrentJob();
    if(!currentJob)
    {
        text->clear();
        text->addString(_("No current job"), COLOR_YELLOW);
        return;
    }

    ss << "Jobs to do: " << printer->ai->getNumJobs() << std::endl << std::endl;

    const auto* bj = dynamic_cast<const AIJH::BuildJob*>(currentJob);
    const auto* ej = dynamic_cast<const AIJH::EventJob*>(currentJob);

    if(bj)
    {
        ss << "BuildJob:" << std::endl;
        ss << BUILDING_NAMES[bj->getType()] << std::endl;
        ss << bj->getTarget().x << " / " << bj->getTarget().y << std::endl;
    } else if(ej)
    {
#define RTTR_PRINT_EV(ev) \
    case AIEvent::EventType::ev: ss << #ev << std::endl; break
        switch(ej->getEvent().getType())
        {
            RTTR_PRINT_EV(BuildingDestroyed);
            RTTR_PRINT_EV(BuildingConquered);
            RTTR_PRINT_EV(BuildingLost);
            RTTR_PRINT_EV(BorderChanged);
            RTTR_PRINT_EV(NoMoreResourcesReachable);
            RTTR_PRINT_EV(BuildingFinished);
            RTTR_PRINT_EV(ExpeditionWaiting);
            RTTR_PRINT_EV(TreeChopped);
            RTTR_PRINT_EV(ShipBuilt);
            RTTR_PRINT_EV(ResourceUsed);
            RTTR_PRINT_EV(RoadConstructionComplete);
            RTTR_PRINT_EV(RoadConstructionFailed);
            RTTR_PRINT_EV(NewColonyFounded);
            RTTR_PRINT_EV(LuaConstructionOrder);
            RTTR_PRINT_EV(ResourceFound);
            RTTR_PRINT_EV(LostLand);
            default: ss << "Unknown Event" << std::endl; break;
        }
#undef RTTR_PRINT_EV

        const auto* evb = dynamic_cast<const AIEvent::Building*>(&ej->getEvent());
        if(evb)
        {
            ss << evb->getX() << " / " << evb->getY() << std::endl;
            ss << BUILDING_NAMES[evb->getBuildingType()] << std::endl;
        }
    }

#define RTTR_PRINT_STATUS(state) \
    case AIJH::state: ss << #state << std::endl; break
    switch(currentJob->getState())
    {
        RTTR_PRINT_STATUS(JobState::Waiting);
        RTTR_PRINT_STATUS(JobState::Start);
        RTTR_PRINT_STATUS(JobState::ExecutingRoad1);
        RTTR_PRINT_STATUS(JobState::ExecutingRoad2);
        RTTR_PRINT_STATUS(JobState::ExecutingRoad2_2);
        RTTR_PRINT_STATUS(JobState::Finished);
        RTTR_PRINT_STATUS(JobState::Failed);
        default: ss << "Unknown status"; break;
    }

    text->clear();
    text->addString(ss.str(), COLOR_YELLOW);
}
