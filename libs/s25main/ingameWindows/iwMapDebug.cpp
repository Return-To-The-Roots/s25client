// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "iwMapDebug.h"
#include "GamePlayer.h"
#include "Loader.h"
#include "PointOutput.h"
#include "RttrForeachPt.h"
#include "ai/aijh/AIPlayerJH.h"
#include "controls/ctrlCheck.h"
#include "controls/ctrlComboBox.h"
#include "controls/ctrlEdit.h"
#include "controls/ctrlTimer.h"
#include "driver/KeyEvent.h"
#include "helpers/toString.h"
#include "notifications/NodeNote.h"
#include "ogl/glFont.h"
#include "world/GameWorldBase.h"
#include "world/GameWorldView.h"
#include "world/NodeMapBase.h"
#include "world/TerritoryRegion.h"
#include "gameTypes/GameTypesOutput.h"
#include "gameTypes/TextureColor.h"
#include "gameData/const_gui_ids.h"
#include "s25util/StringConversion.h"
#include <boost/nowide/iostream.hpp>
#include <chrono>

namespace {
enum
{
    ID_cbShowCoordinates,
    ID_cbShowWhat,
    ID_cbShowForPlayer,
    ID_cbCheckEventForPlayer,
    ID_tmrCheckEvents,
    ID_lblJump,
    ID_edtJumpX,
    ID_edtJumpY,
};
}

class iwMapDebug::DebugPrinter : public IDrawNodeCallback
{
public:
    DebugPrinter(const GameWorldBase& gwb) : showCoords(true), showDataIdx(0), playerIdx(0), gw(gwb), font(NormalFont)
    {}

    void onDraw(const MapPoint& pt, const DrawPoint& displayPt) override
    {
        std::string data;
        unsigned coordsColor = 0xFFFFFF00;
        unsigned dataColor = 0xFFFF0000;
        const MapNode& node = gw.GetNode(pt);
        switch(showDataIdx)
        {
            case 1:
                if(node.reserved)
                    data = "R";
                break;
            case 2: data = helpers::toString(node.altitude); break;
            case 3: data = helpers::toString(node.resources.getValue()); break;
            case 4:
                if(node.seaId)
                    data = helpers::toString(node.seaId);
                else if(gw.GetSeaFromCoastalPoint(pt))
                    data = "C";
                break;
            case 5: data = helpers::toString(node.owner); break;
            case 6:
            {
                bool isAllowed =
                  TerritoryRegion::IsPointValid(gw.GetSize(), gw.GetPlayer(playerIdx).GetRestrictedArea(), pt);
                coordsColor = dataColor = isAllowed ? 0xFF00FF00 : 0xFFFF0000;
                if(!showCoords)
                    data = isAllowed ? "y" : "n";
                break;
            }
        }

        if(showCoords)
        {
            std::string coord = helpers::toString(pt.x) + ":" + helpers::toString(pt.y);
            font->Draw(displayPt, coord, FontStyle{}, coordsColor);
        }
        if(!data.empty())
            font->Draw(displayPt, data, FontStyle{}, dataColor);
    }

    bool showCoords;
    unsigned showDataIdx;
    unsigned playerIdx;
    const GameWorldBase& gw;
    const glFont* font;
};

class iwMapDebug::EventChecker
{
public:
    EventChecker(const GameWorldBase& gw) : gw(gw)
    {
        bqMap.Resize(gw.GetSize());
        for(unsigned i = 0; i < gw.GetNumPlayers(); i++)
        {
            if(gw.GetPlayer(i).isUsed())
                usedPlayerIdxs.push_back(i);
        }
        RTTR_FOREACH_PT(MapPoint, bqMap.GetSize())
        {
            for(const unsigned playerIdx : usedPlayerIdxs)
                bqMap[pt][playerIdx] = gw.GetBQ(pt, playerIdx);
        }
        nodeSub = AIJH::recordBQsToUpdate(this->gw, this->pointsToUpdate);
    }
    void check()
    {
        for(const MapPoint pt : pointsToUpdate)
            updateBq(pt);
        pointsToUpdate.clear();
        RTTR_FOREACH_PT(MapPoint, bqMap.GetSize())
        {
            for(const unsigned playerIdx : usedPlayerIdxs)
            {
                if(bqMap[pt][playerIdx] != gw.GetBQ(pt, playerIdx))
                {
                    boost::nowide::cerr << "BQs mismatch at " << pt << " for player " << playerIdx << ": "
                                        << bqMap[pt][playerIdx] << "!=" << gw.GetBQ(pt, playerIdx) << std::endl;
                }
            }
        }
    }

private:
    void updateBq(MapPoint pt)
    {
        for(const unsigned playerIdx : usedPlayerIdxs)
            bqMap[pt][playerIdx] = gw.GetBQ(pt, playerIdx);
    }
    const GameWorldBase& gw;
    std::vector<unsigned> usedPlayerIdxs;
    using BqNode = std::array<BuildingQuality, MAX_PLAYERS>;
    NodeMapBase<BqNode> bqMap;
    std::vector<MapPoint> pointsToUpdate;
    Subscription nodeSub;
};

using namespace std::chrono_literals;
static const std::array<std::chrono::milliseconds, 6> BQ_CHECK_INTERVALS = {10s, 1s, 500ms, 250ms, 100ms, 50ms};

iwMapDebug::iwMapDebug(GameWorldView& gwv, bool allowCheating)
    : IngameWindow(CGI_MAP_DEBUG, IngameWindow::posLastOrCenter, Extent(230, 160), _("Map Debug"),
                   LOADER.GetImageN("resource", 41)),
      gwv(gwv), printer(std::make_unique<DebugPrinter>(gwv.GetWorld()))
{
    gwv.AddDrawNodeCallback(printer.get());

    DrawPoint curPos(15, 25);
    constexpr Extent ctrlSize(200, 20);
    constexpr auto spaceY = ctrlSize.y + 5;

    ctrlCheck* cbShowCoords =
      AddCheckBox(ID_cbShowCoordinates, curPos, ctrlSize, TextureColor::Grey, _("Show coordinates"), NormalFont);
    curPos.y += spaceY;
    cbShowCoords->setChecked(true);

    constexpr Extent editSize(40, ctrlSize.y);
    AddText(ID_lblJump, curPos + DrawPoint(0, 2), _("Jump to X:Y"), COLOR_YELLOW, FontStyle{}, NormalFont);
    const DrawPoint edtPos = curPos + DrawPoint(ctrlSize.x - 2 * editSize.x - 2, 0);
    AddEdit(ID_edtJumpX, edtPos, editSize, TextureColor::Grey, NormalFont, 4);
    AddEdit(ID_edtJumpY, edtPos + DrawPoint(editSize.x + 2, 0), editSize, TextureColor::Grey, NormalFont, 4);
    curPos.y += spaceY;

    ctrlComboBox* cbCheckEvents =
      AddComboBox(ID_cbCheckEventForPlayer, curPos, ctrlSize, TextureColor::Grey, NormalFont, 100);
    curPos.y += spaceY;
    cbCheckEvents->AddString(_("BQ check disabled"));
    for(const std::chrono::milliseconds ms : BQ_CHECK_INTERVALS)
    {
        cbCheckEvents->AddString((boost::format(_("BQ check every %1%ms")) % ms.count()).str());
    }
    cbCheckEvents->SetSelection(0);
    using namespace std::chrono_literals;
    AddTimer(ID_tmrCheckEvents, 500ms)->Stop();

    if(allowCheating)
    {
        ctrlComboBox* data = AddComboBox(ID_cbShowWhat, curPos, ctrlSize, TextureColor::Grey, NormalFont, 100);
        curPos.y += spaceY;
        data->AddString(_("Nothing"));
        data->AddString(_("Reserved"));
        data->AddString(_("Altitude"));
        data->AddString(_("Resources"));
        data->AddString(_("Sea Id"));
        data->AddString(_("Owner"));
        data->AddString(_("Restricted area"));
        data->SetSelection(1);
        ctrlComboBox* players = AddComboBox(ID_cbShowForPlayer, curPos, ctrlSize, TextureColor::Grey, NormalFont, 100);
        curPos.y += spaceY;
        for(unsigned pIdx = 0; pIdx < gwv.GetWorld().GetNumPlayers(); pIdx++)
        {
            const GamePlayer& p = gwv.GetWorld().GetPlayer(pIdx);
            if(!p.isUsed())
                players->AddString((boost::format(_("Player %1%")) % pIdx).str());
            else
                players->AddString(p.name);
        }
        players->SetSelection(0);
        printer->showDataIdx = data->GetSelection().get();
        printer->playerIdx = players->GetSelection().get();
    } else
    {
        printer->showDataIdx = 0;
        printer->playerIdx = 0;
        Extent iwSize = GetIwSize();
        iwSize.y -= 40 + 10;
        SetIwSize(iwSize);
    }

    printer->showCoords = cbShowCoords->isChecked();
}

iwMapDebug::~iwMapDebug()
{
    gwv.RemoveDrawNodeCallback(printer.get());
}

void iwMapDebug::Msg_ComboSelectItem(const unsigned ctrl_id, const unsigned select)
{
    if(ctrl_id == ID_cbShowWhat)
        printer->showDataIdx = select;
    else if(ctrl_id == ID_cbShowForPlayer)
        printer->playerIdx = select;
    else if(ctrl_id == ID_cbCheckEventForPlayer)
    {
        if(select == 0)
        {
            eventChecker.reset();
            GetCtrl<ctrlTimer>(ID_tmrCheckEvents)->Stop();
        } else
        {
            eventChecker = std::make_unique<EventChecker>(gwv.GetWorld());
            GetCtrl<ctrlTimer>(ID_tmrCheckEvents)->Start(BQ_CHECK_INTERVALS[select - 1]);
        }
    }
}

void iwMapDebug::Msg_CheckboxChange(const unsigned ctrl_id, const bool checked)
{
    if(ctrl_id == ID_cbShowCoordinates)
        printer->showCoords = checked;
}

void iwMapDebug::Msg_Timer(unsigned /*ctrl_id*/)
{
    if(eventChecker)
        eventChecker->check();
}

void iwMapDebug::Msg_EditEnter(unsigned ctrl_id)
{
    RTTR_Assert(ctrl_id == ID_edtJumpX || ctrl_id == ID_edtJumpY);
    Position pos;
    if(s25util::tryFromStringClassic(GetCtrl<ctrlEdit>(ID_edtJumpX)->GetText(), pos.x)
       && s25util::tryFromStringClassic(GetCtrl<ctrlEdit>(ID_edtJumpY)->GetText(), pos.y))
    {
        gwv.MoveToMapPt(gwv.GetWorld().MakeMapPoint(pos));
    }
}

bool iwMapDebug::Msg_KeyDown(const KeyEvent& ke)
{
    switch(ke.kt)
    {
        case KeyType::Return: Msg_EditEnter(ID_edtJumpY); break;
        case KeyType::Tab:
        {
            auto* edtNewFocus = GetCtrl<ctrlEdit>(ID_edtJumpX);
            if(edtNewFocus->HasFocus())
                edtNewFocus = GetCtrl<ctrlEdit>(ID_edtJumpY);
            edtNewFocus->SetFocus();
            edtNewFocus->SetText("");
        };
        break;
        default: return IngameWindow::Msg_KeyDown(ke);
    }
    return true;
}

void iwMapDebug::SetActive(bool activate)
{
    IngameWindow::SetActive(activate);
    if(activate)
        GetCtrl<ctrlEdit>(ID_edtJumpX)->SetFocus();
}
