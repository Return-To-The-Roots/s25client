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

#include "iwWares.h"
#include "GamePlayer.h"
#include "Loader.h"
#include "WindowManager.h"
#include "controls/ctrlButton.h"
#include "controls/ctrlGroup.h"
#include "controls/ctrlImage.h"
#include "controls/ctrlText.h"
#include "iwHelp.h"
#include "ogl/FontStyle.h"
#include "gameData/GoodConsts.h"
#include "gameData/JobConsts.h"
#include "gameData/ShieldConsts.h"

namespace {
constexpr unsigned ID_pageOffset = 100;
}

static void addElement(ctrlGroup& page, const glFont* font, const DrawPoint btPos, const Extent btSize,
                       const unsigned idOffset, const std::string& name, ITexture* img, const bool allow_outhousing)
{
    // Background image, only a button when outhousing is allowed
    if(allow_outhousing)
    {
        ctrlButton* b = page.AddImageButton(100 + idOffset, btPos, btSize, TC_GREY, LOADER.GetMapImageN(2298), name);
        b->SetBorder(false);
    } else
        page.AddImage(100 + idOffset, btPos + btSize / 2, LOADER.GetMapImageN(2298), name);

    // Background image for the amount
    const DrawPoint bgCtPos = btPos + DrawPoint(btSize.x / 2, 32);
    page.AddImage(200 + idOffset, bgCtPos, LOADER.GetMapImageN(2299));

    // The actual image for the element
    const DrawPoint warePos = btPos + btSize / 2;
    page.AddImage(300 + idOffset, warePos, img);

    // Overlay for "don't collect"
    DrawPoint overlayPos = warePos - DrawPoint(0, 4);
    ctrlImage* image = page.AddImage(400 + idOffset, overlayPos, LOADER.GetImageN("io", 222));
    image->SetVisible(false);

    // Overlay for "send out"
    overlayPos = warePos + DrawPoint(0, 10);
    image = page.AddImage(500 + idOffset, overlayPos, LOADER.GetImageN("io", 221));
    image->SetVisible(false);

    // Overlay for "collect"
    image = page.AddImage(700 + idOffset, overlayPos, LOADER.GetImageN("io_new", 3));
    image->SetVisible(false);

    // Amount of the element
    const DrawPoint txtPos = btPos + DrawPoint(btSize.x, 40);
    page.AddText(600 + idOffset, txtPos, "", COLOR_YELLOW, FontStyle::RIGHT | FontStyle::BOTTOM, font);
}

// 167, 416
iwWares::iwWares(unsigned id, const DrawPoint& pos, const Extent& size, const std::string& title, bool allow_outhousing,
                 const glFont* font, const Inventory& inventory, const GamePlayer& player)
    : IngameWindow(id, pos, size, title, LOADER.GetImageN("io", 5)), inventory(inventory), player(player), numPages(0)
{
    if(!font)
        font = SmallFont;

    // Zuordnungs-IDs
    constexpr std::array<GoodType, 31> WARE_DISPLAY_ORDER{
      GoodType::Wood,    GoodType::Boards,   GoodType::Stones,
      GoodType::Ham,     GoodType::Grain,    GoodType::Flour,
      GoodType::Fish,    GoodType::Meat,     GoodType::Bread,
      GoodType::Water,   GoodType::Beer,     GoodType::Coal,
      GoodType::IronOre, GoodType::Gold,     GoodType::Iron,
      GoodType::Coins,   GoodType::Tongs,    GoodType::Axe,
      GoodType::Saw,     GoodType::PickAxe,  GoodType::Hammer,
      GoodType::Shovel,  GoodType::Crucible, GoodType::RodAndLine,
      GoodType::Scythe,  GoodType::Cleaver,  GoodType::Rollingpin,
      GoodType::Bow,     GoodType::Sword,    GoodType::ShieldRomans /* nation specific */,
      GoodType::Boat};

    constexpr std::array<Job, 31> JOB_DISPLAY_ORDER{
      Job::Helper,      Job::Builder,     Job::Planer,     Job::Woodcutter,
      Job::Forester,    Job::Stonemason,  Job::Fisher,     Job::Hunter,
      Job::Carpenter,   Job::Farmer,      Job::PigBreeder, Job::DonkeyBreeder,
      Job::Miller,      Job::Baker,       Job::Butcher,    Job::Brewer,
      Job::Miner,       Job::IronFounder, Job::Armorer,    Job::Minter,
      Job::Metalworker, Job::Shipwright,  Job::Geologist,  Job::Scout,
      Job::PackDonkey,  Job::CharBurner,  Job::Private,    Job::PrivateFirstClass,
      Job::Sergeant,    Job::Officer,     Job::General};

    // Warenseite hinzufügen
    ctrlGroup& waresPage = AddPage();
    warePageID = waresPage.GetID();
    // Figurenseite hinzufügen
    ctrlGroup& figuresPage = AddPage();
    peoplePageID = figuresPage.GetID();

    bool isRowWithFourElemens = true;
    constexpr unsigned numElements = std::max(WARE_DISPLAY_ORDER.size(), JOB_DISPLAY_ORDER.size());
    for(unsigned idx = 0, x = 0, y = 0; idx < numElements; ++x, ++idx)
    {
        // Alternating rows with 4 and 5 items
        if(x >= (isRowWithFourElemens ? 4u : 5u))
        {
            x = 0;
            ++y;

            isRowWithFourElemens = !isRowWithFourElemens;
        }

        const Extent btSize(26, 26);
        const DrawPoint btPos((isRowWithFourElemens ? btSize.x + 1 : btSize.x / 2) + x * 28, 21 + y * 42);

        if(idx < WARE_DISPLAY_ORDER.size())
        {
            const GoodType rawWare = WARE_DISPLAY_ORDER[idx];
            const GoodType ware = convertShieldToNation(rawWare, player.nation);
            addElement(waresPage, font, btPos, btSize, rttr::enum_cast(rawWare), _(WARE_NAMES[rawWare]),
                       LOADER.GetWareTex(ware), allow_outhousing);
        }

        if(idx < JOB_DISPLAY_ORDER.size())
        {
            const Job job = JOB_DISPLAY_ORDER[idx];
            addElement(figuresPage, font, btPos, btSize, rttr::enum_cast(job), _(JOB_NAMES[job]), LOADER.GetJobTex(job),
                       allow_outhousing);
        }
    }

    // "Blättern"
    AddImageButton(0, DrawPoint(52, GetSize().y - 47), Extent(66, 32), TC_GREY, LOADER.GetImageN("io", 84),
                   _("Next page"));
    // Hilfe
    AddImageButton(12, DrawPoint(16, GetSize().y - 47), Extent(32, 32), TC_GREY, LOADER.GetImageN("io", 225),
                   _("Help"));

    waresPage.SetVisible(true);
    curPage_ = warePageID;
}

void iwWares::Msg_ButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case 0: // "Blättern"
            SetPage(curPage_ + 1);
            break;
        case 12: // Hilfe
            WINDOWMANAGER.ReplaceWindow(
              std::make_unique<iwHelp>(_("Here you will find a list of your entire stores of "
                                         "merchandise and all the inhabitants of your realm.")));
            break;
    }
}

void iwWares::Msg_PaintBefore()
{
    IngameWindow::Msg_PaintBefore();

    // Farben ggf. aktualisieren

    if(curPage_ != peoplePageID && curPage_ != warePageID)
        return;

    auto* group = GetCtrl<ctrlGroup>(curPage_);
    if(group)
    {
        const unsigned count =
          (curPage_ == warePageID) ? helpers::NumEnumValues_v<GoodType> : helpers::NumEnumValues_v<Job>;

        for(unsigned i = 0; i < count; ++i)
        {
            auto* text = group->GetCtrl<ctrlText>(600 + i);
            if(text)
            {
                const unsigned amount =
                  (curPage_ == warePageID) ? inventory[static_cast<GoodType>(i)] : inventory[static_cast<Job>(i)];
                text->SetText(std::to_string(amount));
                text->SetTextColor((amount == 0) ? COLOR_RED : COLOR_YELLOW);
            }
        }
    }
}

/**
 *  bestimmte Inventurseite zeigen.
 *
 *  @param[in] page Die neue Seite
 */
void iwWares::SetPage(unsigned page)
{
    // alte Page verstecken
    auto* group = GetCtrl<ctrlGroup>(curPage_);
    if(group)
        group->SetVisible(false);

    // neue Page setzen
    curPage_ = (page - ID_pageOffset) % numPages + ID_pageOffset;

    // neue Page anzeigen
    group = GetCtrl<ctrlGroup>(curPage_);
    if(group)
        group->SetVisible(true);
}

ctrlGroup& iwWares::AddPage()
{
    ctrlGroup& grp = *AddGroup(ID_pageOffset + numPages);
    numPages++;
    grp.SetVisible(false);
    return grp;
}
