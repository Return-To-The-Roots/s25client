// Copyright (C) 2005 - 2023 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "dskCampaignSelection.h"
#include "ListDir.h"
#include "Loader.h"
#include "RttrConfig.h"
#include "WindowManager.h"
#include "controls/ctrlButton.h"
#include "controls/ctrlMultiline.h"
#include "controls/ctrlTable.h"
#include "controls/ctrlText.h"
#include "controls/ctrlTimer.h"
#include "dskCampaignMissionSelection.h"
#include "dskSinglePlayer.h"
#include "files.h"
#include "helpers/Range.h"
#include "helpers/containerUtils.h"
#include "helpers/format.hpp"
#include "ingameWindows/iwMsgbox.h"
#include "lua/CampaignDataLoader.h"
#include "ogl/glFont.h"
#include "gameData/CampaignDescription.h"
#include "libsiedler2/Archiv.h"
#include "libsiedler2/ErrorCodes.h"
#include "libsiedler2/prototypen.h"
#include "s25util/Log.h"
#include <boost/filesystem/operations.hpp>
#include <chrono>

namespace bfs = boost::filesystem;
using namespace std::chrono_literals;

namespace {
enum
{
    ID_Label,
    ID_Table,
    ID_PreviewTitle,
    ID_PreviewDescription,
    ID_Back,
    ID_Next,
    ID_TimerFillCampaignTable
};

constexpr int startOffsetY = 20;
constexpr int padding = 10;
constexpr int firstColumnExtentX = 480;
constexpr int secondColumnExtentX = 280;
constexpr int columnExtentY = 500;
constexpr int secondColumnOffsetX = padding * 2 + firstColumnExtentX;
constexpr int campaignImageExtentY = 220;

int getColumnOffsetY()
{
    return startOffsetY + LargeFont->getHeight();
}

} // namespace

dskCampaignSelection::dskCampaignSelection(CreateServerInfo csi)
    : Desktop(LOADER.GetImageN("setup015", 0)), showCampaignInfo_(false), csi_(std::move(csi))
{
    AddText(ID_Label, DrawPoint(250, startOffsetY), _("Campaigns"), COLOR_YELLOW, FontStyle::CENTER, LargeFont);

    // The table for the campaigns
    using SRT = ctrlTable::SortType;
    AddTable(ID_Table, DrawPoint(padding, getColumnOffsetY()), Extent(firstColumnExtentX, columnExtentY),
             TextureColor::Grey, NormalFont,
             ctrlTable::Columns{{_("Title"), 100, SRT::String},
                                {_("Description"), 150, SRT::String},
                                {_("Author"), 100, SRT::String},
                                {_("Maps"), 20, SRT::Number},
                                {_("Difficulty"), 40, SRT::String},
                                {"", 0, SRT::Default}});

    const int previewTitleOffsetY = getColumnOffsetY() + campaignImageExtentY + padding;
    AddText(ID_PreviewTitle, DrawPoint(640, previewTitleOffsetY), "", COLOR_YELLOW, FontStyle::CENTER, LargeFont);

    int const multilineOffsetY = previewTitleOffsetY + LargeFont->getHeight();
    int const mutlilineExtentY = columnExtentY - (multilineOffsetY - getColumnOffsetY());
    AddMultiline(ID_PreviewDescription, DrawPoint(secondColumnOffsetX, multilineOffsetY),
                 Extent(secondColumnExtentX, mutlilineExtentY), TextureColor::Grey, NormalFont);

    AddTextButton(ID_Back, DrawPoint(380, 560), Extent(200, 22), TextureColor::Red1, _("Back"), NormalFont);
    AddTextButton(ID_Next, DrawPoint(590, 560), Extent(200, 22), TextureColor::Green2, _("Continue"), NormalFont);

    showCampaignInfo(false);

    // Delay loading until screen is shown such that possible error message boxes are shown after switching to this
    AddTimer(ID_TimerFillCampaignTable, 1ms);
}

void dskCampaignSelection::Msg_TableChooseItem(const unsigned ctrl_id, const unsigned)
{
    if(ctrl_id == ID_Table)
        WINDOWMANAGER.Switch(std::make_unique<dskCampaignMissionSelection>(csi_, getSelectedCampaignPath()));
}

void dskCampaignSelection::Msg_TableSelectItem(const unsigned ctrl_id, const boost::optional<unsigned>& selection)
{
    if(ctrl_id != ID_Table)
        return;

    ctrlButton& btContinue = *GetCtrl<ctrlButton>(ID_Next);
    btContinue.SetEnabled(false);
    showCampaignInfo(false);
    campaignImage_ = nullptr;

    if(selection)
    {
        ctrlTable& campaignTable = *GetCtrl<ctrlTable>(ctrl_id);
        const std::string& campaignFolder = campaignTable.GetItemText(*selection, 5);

        CampaignDescription desc;
        CampaignDataLoader loader(desc, campaignFolder);
        if(loader.Load())
        {
            ctrlMultiline& multiline = *GetCtrl<ctrlMultiline>(ID_PreviewDescription);
            ctrlText& campaignTitle = *GetCtrl<ctrlText>(ID_PreviewTitle);
            multiline.Clear();
            multiline.AddString(desc.longDescription, COLOR_YELLOW);
            campaignTitle.SetText(desc.name);

            if(!desc.image.empty() && LOADER.LoadFiles({desc.image}))
            {
                campaignImage_ = LOADER.GetImageN(ResourceId::make(RTTRCONFIG.ExpandPath(desc.image)), 0);
            }
            btContinue.SetEnabled(true);
            showCampaignInfo(true);
        }
    }
}

void dskCampaignSelection::showCampaignInfo(bool show)
{
    GetCtrl<ctrlMultiline>(ID_PreviewDescription)->SetVisible(show);
    GetCtrl<ctrlText>(ID_PreviewTitle)->SetVisible(show);
    showCampaignInfo_ = show;
}

boost::filesystem::path dskCampaignSelection::getSelectedCampaignPath()
{
    auto* table = GetCtrl<ctrlTable>(ID_Table);
    const auto& selection = table->GetSelection();
    if(!selection)
        throw std::runtime_error("Campaign path is empty.");

    boost::filesystem::path campaignPath = table->GetItemText(*selection, 5);
    RTTR_Assert(!campaignPath.empty());
    return campaignPath;
}

void dskCampaignSelection::Msg_ButtonClick(unsigned ctrl_id)
{
    if(ctrl_id == ID_Back)
        WINDOWMANAGER.Switch(std::make_unique<dskSinglePlayer>());
    else if(ctrl_id == ID_Next)
        WINDOWMANAGER.Switch(std::make_unique<dskCampaignMissionSelection>(csi_, getSelectedCampaignPath()));
}

void dskCampaignSelection::Msg_Timer(unsigned ctrl_id)
{
    GetCtrl<ctrlTimer>(ctrl_id)->Stop();
    FillCampaignsTable();
}

void dskCampaignSelection::FillCampaignsTable()
{
    const size_t numFaultyCampaignsPrior = brokenCampaignPaths_.size();
    static const std::array<std::string, 2> campaignFolders = {
      {s25::folders::campaignsBuiltin, s25::folders::campaignsUser}};

    auto* table = GetCtrl<ctrlTable>(ID_Table);
    for(const auto& campaignFolder : campaignFolders)
    {
        for(const bfs::path& folder : ListDir(RTTRCONFIG.ExpandPath(campaignFolder), "", true))
        {
            if(!is_directory(folder) || helpers::contains(brokenCampaignPaths_, folder))
                continue;

            CampaignDescription desc;
            CampaignDataLoader loader(desc, folder);
            if(!loader.Load())
            {
                LOG.write(_("Failed to load campaign %1%.\n")) % folder;
                brokenCampaignPaths_.insert(folder);
                continue;
            }

            bool success = true;
            for(auto idx : helpers::range(desc.getNumMaps()))
            {
                auto const mapPath = desc.getMapFilePath(idx);
                auto const luaFilepath = desc.getLuaFilePath(idx);
                if(!bfs::exists(mapPath))
                {
                    LOG.write(_("Campaign map %1% does not exist.\n")) % mapPath;
                    brokenCampaignPaths_.insert(folder);
                    success = false;
                    continue;
                }
                if(!bfs::exists(luaFilepath))
                {
                    LOG.write(_("Campaign map lua file %1% does not exist.\n")) % luaFilepath;
                    brokenCampaignPaths_.insert(folder);
                    success = false;
                    continue;
                }

                libsiedler2::Archiv map;
                if(int ec = libsiedler2::loader::LoadMAP(mapPath, map, true))
                {
                    LOG.write(_("Failed to load map %1%: %2%\n")) % mapPath % libsiedler2::getErrorString(ec);
                    brokenCampaignPaths_.insert(folder);
                    success = false;
                    continue;
                }
            }

            if(success)
                table->AddRow({desc.name, desc.shortDescription, desc.author, std::to_string(desc.getNumMaps()),
                               _(desc.difficulty), folder.string()});
        }
    }

    if(brokenCampaignPaths_.size() > numFaultyCampaignsPrior)
    {
        std::string errorTxt = helpers::format(_("%1% campaign(s) could not be loaded. Check the log for details"),
                                               brokenCampaignPaths_.size() - numFaultyCampaignsPrior);
        WINDOWMANAGER.Show(
          std::make_unique<iwMsgbox>(_("Error"), errorTxt, this, MsgboxButton::Ok, MsgboxIcon::ExclamationRed, 1));
    }
}

void dskCampaignSelection::Draw_()
{
    Desktop::Draw_();

    // replace this by AddImageButton as soon as it can handle this sceneraio correctly
    if(showCampaignInfo_ && campaignImage_)
    {
        campaignImage_->DrawFull(Rect(ScaleIf(DrawPoint(secondColumnOffsetX, getColumnOffsetY())),
                                      ScaleIf(Extent(secondColumnExtentX, campaignImageExtentY))));
    }
}
