// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "dskCampaignSelection.h"
#include "ListDir.h"
#include "Loader.h"
#include "RttrConfig.h"
#include "WindowManager.h"
#include "controls/ctrlButton.h"
#include "controls/ctrlMapSelection.h"
#include "controls/ctrlMultiline.h"
#include "controls/ctrlTable.h"
#include "controls/ctrlText.h"
#include "controls/ctrlTimer.h"
#include "dskCampaignMissionSelection.h"
#include "dskSinglePlayer.h"
#include "files.h"
#include "helpers/Range.h"
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
#include <set>

namespace bfs = boost::filesystem;
using namespace std::chrono_literals;

namespace {
enum
{
    ID_Label,
    ID_Table,
    ID_PreviewTitle,
    ID_PreviewDescription,
    ID_btBack,
    ID_Next,
    ID_TimerFillCampaignTable,
    ID_MapSelection
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
    : Desktop(LOADER.getImageN("setup015", 0)), csi_(std::move(csi))
{
    addText(ID_Label, DrawPoint(250, startOffsetY), _("Campaigns"), COLOR_YELLOW, FontStyle::CENTER, LargeFont);

    // The table for the campaigns
    using SRT = ctrlTable::SortType;
    addTable(ID_Table, DrawPoint(padding, getColumnOffsetY()), Extent(firstColumnExtentX, columnExtentY),
             TextureColor::Grey, NormalFont,
             ctrlTable::Columns{{_("Title"), 100, SRT::String},
                                {_("Description"), 150, SRT::String},
                                {_("Author"), 60, SRT::String},
                                {_("Maps"), 40, SRT::Number},
                                {_("Difficulty"), 70, SRT::String},
                                {"", 0, SRT::Default}});

    const int previewTitleOffsetY = getColumnOffsetY() + campaignImageExtentY + padding;
    addText(ID_PreviewTitle, DrawPoint(640, previewTitleOffsetY), "", COLOR_YELLOW, FontStyle::CENTER, LargeFont);

    int const multilineOffsetY = previewTitleOffsetY + LargeFont->getHeight();
    int const mutlilineExtentY = columnExtentY - (multilineOffsetY - getColumnOffsetY());
    addMultiline(ID_PreviewDescription, DrawPoint(secondColumnOffsetX, multilineOffsetY),
                 Extent(secondColumnExtentX, mutlilineExtentY), TextureColor::Grey, NormalFont);

    addTextButton(ID_btBack, DrawPoint(380, 560), Extent(200, 22), TextureColor::Red1, _("Back"), NormalFont);
    addTextButton(ID_Next, DrawPoint(590, 560), Extent(200, 22), TextureColor::Green2, _("Continue"), NormalFont)
      ->setEnabled(false);

    showCampaignInfo(false);

    // Delay loading until screen is shown such that possible error message boxes are shown after switching to this
    addTimer(ID_TimerFillCampaignTable, 1ms);
}

dskCampaignSelection::~dskCampaignSelection() noexcept = default;

void dskCampaignSelection::msgTableChooseItem(const unsigned ctrl_id, const unsigned)
{
    if(ctrl_id == ID_Table)
        showCampaignMissionSelectionScreen();
}

void dskCampaignSelection::msgTableSelectItem(const unsigned ctrl_id, const std::optional<unsigned>& selection)
{
    if(ctrl_id != ID_Table)
        return;

    deleteCtrl(ID_MapSelection); // Needs to be either hidden or recreated as it doesn't support updating
    auto& btContinue = *getCtrl<ctrlButton>(ID_Next);
    if(!selection)
    {
        btContinue.setEnabled(false);
        showCampaignInfo(false);
    } else
    {
        btContinue.setEnabled(true);
        showCampaignInfo(true);

        const auto& campaign = campaigns_.at(*selection);

        auto& campaignDescription = *getCtrl<ctrlMultiline>(ID_PreviewDescription);
        campaignDescription.clear();
        campaignDescription.addString(campaign.longDescription, COLOR_YELLOW);

        auto& campaignTitle = *getCtrl<ctrlText>(ID_PreviewTitle);
        campaignTitle.setText(campaign.name);

        if(campaign.selectionMapData)
        {
            campaignImage_ = nullptr;
            auto* mapSelection =
              addMapSelection(ID_MapSelection, DrawPoint(secondColumnOffsetX, getColumnOffsetY()),
                              Extent(secondColumnExtentX, campaignImageExtentY), *campaign.selectionMapData);
            mapSelection->setMissionsStatus(std::vector<MissionStatus>(campaign.getNumMaps(), {true, true}));
            mapSelection->setPreview(true);
        } else if(campaign.image)
            campaignImage_ = LOADER.getImageN(ResourceId::fromPath(*campaign.image), 0);
        else
            campaignImage_ = nullptr;
    }
}

void dskCampaignSelection::showCampaignInfo(const bool show)
{
    getCtrl<ctrlMultiline>(ID_PreviewDescription)->setVisible(show);
    getCtrl<ctrlText>(ID_PreviewTitle)->setVisible(show);
    if(!show)
        campaignImage_ = nullptr;
}

void dskCampaignSelection::showCampaignMissionSelectionScreen()
{
    const auto& campaign = campaigns_.at(getCtrl<ctrlTable>(ID_Table)->getSelection().value());
    WINDOWMANAGER.switchTo(std::make_unique<dskCampaignMissionSelection>(csi_, campaign));
}

void dskCampaignSelection::msgButtonClick(unsigned ctrl_id)
{
    if(ctrl_id == ID_btBack)
        WINDOWMANAGER.switchTo(std::make_unique<dskSinglePlayer>());
    else if(ctrl_id == ID_Next)
        showCampaignMissionSelectionScreen();
}

void dskCampaignSelection::msgTimer(unsigned ctrl_id)
{
    getCtrl<ctrlTimer>(ctrl_id)->stop();
    loadCampaigns();
    std::set<std::string> resourcesToLoad;
    for(const auto& campaign : campaigns_)
    {
        if(campaign.image)
            resourcesToLoad.insert(*campaign.image);
    }
    LOADER.loadFiles({resourcesToLoad.begin(), resourcesToLoad.end()});
    fillCampaignsTable();
}

void dskCampaignSelection::fillCampaignsTable()
{
    auto* table = getCtrl<ctrlTable>(ID_Table);
    for(const auto& campaign : campaigns_)
    {
        table->addRow({campaign.name, campaign.shortDescription, campaign.author, std::to_string(campaign.getNumMaps()),
                       _(campaign.difficulty)});
    }
}

void dskCampaignSelection::drawImpl()
{
    Desktop::drawImpl();

    // replace this by addImageButton as soon as it can handle this scenario correctly
    if(campaignImage_)
    {
        campaignImage_->drawFull(Rect(scaleIf(DrawPoint(secondColumnOffsetX, getColumnOffsetY())),
                                      scaleIf(Extent(secondColumnExtentX, campaignImageExtentY))));
    }
}

void dskCampaignSelection::loadCampaigns()
{
    unsigned numBroken = 0;
    for(const std::string campaignFolder : {s25::folders::campaignsBuiltin, s25::folders::campaignsUser})
    {
        for(const bfs::path& folder : listDir(RTTRCONFIG.expandPath(campaignFolder), "", true))
        {
            if(!is_directory(folder))
                continue;

            CampaignDescription desc;
            CampaignDataLoader loader(desc, folder);
            if(!loader.load())
            {
                LOG.write(_("Failed to load campaign %1%.\n")) % folder;
                numBroken++;
                continue;
            }

            bool mapsOK = true;
            for(const auto idx : helpers::range(desc.getNumMaps()))
            {
                auto const mapPath = desc.getMapFilePath(idx);
                auto const luaFilepath = desc.getLuaFilePath(idx);
                if(!bfs::exists(mapPath))
                {
                    LOG.write(_("Campaign map %1% does not exist.\n")) % mapPath;
                    mapsOK = false;
                } else
                {
                    libsiedler2::Archiv map;
                    if(int ec = libsiedler2::loader::LoadMAP(mapPath, map, true))
                    {
                        LOG.write(_("Failed to load map %1%: %2%\n")) % mapPath % libsiedler2::getErrorString(ec);
                        mapsOK = false;
                    }
                }
                if(!bfs::exists(luaFilepath))
                {
                    LOG.write(_("Campaign map lua file %1% does not exist.\n")) % luaFilepath;
                    mapsOK = false;
                }
            }

            if(mapsOK)
                campaigns_.push_back(std::move(desc));
            else
                numBroken++;
        }
    }

    if(numBroken > 0)
    {
        const std::string errorTxt =
          helpers::format(_("%1% campaign(s) could not be loaded. Check the log for details"), numBroken);
        WINDOWMANAGER.show(
          std::make_unique<iwMsgbox>(_("Error"), errorTxt, this, MsgboxButton::Ok, MsgboxIcon::ExclamationRed, 1));
    }
}
