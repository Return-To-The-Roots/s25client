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
#include "controls/ctrlVarText.h"
#include "iwHelp.h"
#include "ogl/FontStyle.h"
#include "gameData/JobConsts.h"
#include "gameData/ShieldConsts.h"

// 167, 416
iwWares::iwWares(unsigned id, const DrawPoint& pos, const Extent& size, const std::string& title, bool allow_outhousing, const glFont* font,
                 const Inventory& inventory, const GamePlayer& player)
    : IngameWindow(id, pos, size, title, LOADER.GetImageN("io", 5)), inventory(inventory), player(player), curPage_(0), numPages(0)
{
    if(!font)
        font = SmallFont;

    // Zuordnungs-IDs
    constexpr std::array<GoodType, 31> WARE_DISPLAY_ORDER{
      GD_WOOD,    GD_BOARDS,  GD_STONES,     GD_HAM,    GD_GRAIN,    GD_FLOUR,
      GD_FISH,    GD_MEAT,    GD_BREAD,      GD_WATER,  GD_BEER,     GD_COAL,
      GD_IRONORE, GD_GOLD,    GD_IRON,       GD_COINS,  GD_TONGS,    GD_AXE,
      GD_SAW,     GD_PICKAXE, GD_HAMMER,     GD_SHOVEL, GD_CRUCIBLE, GD_RODANDLINE,
      GD_SCYTHE,  GD_CLEAVER, GD_ROLLINGPIN, GD_BOW,    GD_SWORD,    GD_SHIELDROMANS /* nation specific */,
      GD_BOAT};

    constexpr std::array<Job, 31> JOB_DISPLAY_ORDER{
      JOB_HELPER,     JOB_BUILDER,   JOB_PLANER, JOB_WOODCUTTER,  JOB_FORESTER,      JOB_STONEMASON, JOB_FISHER,
      JOB_HUNTER,     JOB_CARPENTER, JOB_FARMER, JOB_PIGBREEDER,  JOB_DONKEYBREEDER, JOB_MILLER,     JOB_BAKER,
      JOB_BUTCHER,    JOB_BREWER,    JOB_MINER,  JOB_IRONFOUNDER, JOB_ARMORER,       JOB_MINTER,     JOB_METALWORKER,
      JOB_SHIPWRIGHT, JOB_GEOLOGIST, JOB_SCOUT,  JOB_PACKDONKEY,  JOB_CHARBURNER,    JOB_PRIVATE,    JOB_PRIVATEFIRSTCLASS,
      JOB_SERGEANT,   JOB_OFFICER,   JOB_GENERAL};

    // Warenseite hinzufügen
    ctrlGroup& waresPage = AddPage();
    pageWares = waresPage.GetID() - 100;
    // Figurenseite hinzufügen
    ctrlGroup& figuresPage = AddPage();
    pagePeople = figuresPage.GetID() - 100;

    bool four = true;
    unsigned short ware_idx = 0;
    for(int x = 0, y = 0; y < 7; ++x, ++ware_idx)
    {
        // 4er und 5er Block abwechselnd
        if(x >= (four ? 4 : 5))
        {
            x = 0;
            ++y;
            if(y == 7)
                break;

            four = !four;
        }

        // Hintergrundbutton oder -bild hinter Ware, nur beim Auslagern ein Button
        Extent btSize(26, 26);
        DrawPoint btPos((four ? btSize.x + 1 : btSize.x / 2) + x * 28, 21 + y * 42);
        if(allow_outhousing)
        {
            ctrlButton* b = waresPage.AddImageButton(100 + WARE_DISPLAY_ORDER[ware_idx], btPos, btSize, TC_GREY, LOADER.GetMapImageN(2298),
                                                     _(WARE_NAMES[WARE_DISPLAY_ORDER[ware_idx]]));
            b->SetBorder(false);
        } else
            waresPage.AddImage(100 + WARE_DISPLAY_ORDER[ware_idx], btPos + btSize / 2, LOADER.GetMapImageN(2298),
                               _(WARE_NAMES[WARE_DISPLAY_ORDER[ware_idx]]));

        if(allow_outhousing)
        {
            ctrlButton* b = figuresPage.AddImageButton(100 + JOB_DISPLAY_ORDER[ware_idx], btPos, btSize, TC_GREY, LOADER.GetMapImageN(2298),
                                                       _(JOB_NAMES[JOB_DISPLAY_ORDER[ware_idx]]));
            b->SetBorder(false);
        } else
        {
            figuresPage.AddImage(100 + JOB_DISPLAY_ORDER[ware_idx], btPos + btSize / 2, LOADER.GetMapImageN(2298),
                                 _(JOB_NAMES[JOB_DISPLAY_ORDER[ware_idx]]));
        }

        // Hintergrundbild hinter Anzahl
        DrawPoint bgCtPos = btPos + DrawPoint(btSize.x / 2, 32);
        waresPage.AddImage(200 + WARE_DISPLAY_ORDER[ware_idx], bgCtPos, LOADER.GetMapImageN(2299));

        figuresPage.AddImage(200 + JOB_DISPLAY_ORDER[ware_idx], bgCtPos, LOADER.GetMapImageN(2299));

        // die jeweilige Ware
        DrawPoint warePos = btPos + btSize / 2;
        const GoodType ware = convertShieldToNation(WARE_DISPLAY_ORDER[ware_idx], player.nation);
        waresPage.AddImage(300 + WARE_DISPLAY_ORDER[ware_idx], warePos, LOADER.GetMapImageN(2250 + ware));

        glArchivItem_Bitmap* figImage;
        // Exception: charburner
        if(JOB_DISPLAY_ORDER[ware_idx] != JOB_CHARBURNER)
            figImage = LOADER.GetMapImageN(2300 + JOB_DISPLAY_ORDER[ware_idx]);
        else
            figImage = LOADER.GetImageN("io_new", 5);

        figuresPage.AddImage(300 + JOB_DISPLAY_ORDER[ware_idx], warePos, figImage);

        // Overlay für "Nicht Einlagern"
        DrawPoint overlayPos = warePos - DrawPoint(0, 4);
        ctrlImage* image = waresPage.AddImage(400 + WARE_DISPLAY_ORDER[ware_idx], overlayPos, LOADER.GetImageN("io", 222));
        image->SetVisible(false);

        image = figuresPage.AddImage(400 + JOB_DISPLAY_ORDER[ware_idx], overlayPos, LOADER.GetImageN("io", 222));
        image->SetVisible(false);

        // Overlay für "Auslagern"
        overlayPos = warePos + DrawPoint(0, 10);
        image = waresPage.AddImage(500 + WARE_DISPLAY_ORDER[ware_idx], overlayPos, LOADER.GetImageN("io", 221));
        image->SetVisible(false);

        image = figuresPage.AddImage(500 + JOB_DISPLAY_ORDER[ware_idx], overlayPos, LOADER.GetImageN("io", 221));
        image->SetVisible(false);

        // Overlay für "Einlagern"
        image = waresPage.AddImage(700 + WARE_DISPLAY_ORDER[ware_idx], overlayPos, LOADER.GetImageN("io_new", 3));
        image->SetVisible(false);

        image = figuresPage.AddImage(700 + JOB_DISPLAY_ORDER[ware_idx], overlayPos, LOADER.GetImageN("io_new", 3));
        image->SetVisible(false);

        // die jeweilige Anzahl (Texte)
        DrawPoint txtPos = btPos + DrawPoint(btSize.x, 40);
        waresPage.AddVarText(600 + WARE_DISPLAY_ORDER[ware_idx], txtPos, _("%d"), COLOR_YELLOW, FontStyle::RIGHT | FontStyle::BOTTOM, font,
                             1, &inventory.goods[WARE_DISPLAY_ORDER[ware_idx]]);

        figuresPage.AddVarText(600 + JOB_DISPLAY_ORDER[ware_idx], txtPos, _("%d"), COLOR_YELLOW, FontStyle::RIGHT | FontStyle::BOTTOM, font,
                               1, &inventory.people[JOB_DISPLAY_ORDER[ware_idx]]);
    }

    // "Blättern"
    AddImageButton(0, DrawPoint(52, GetSize().y - 47), Extent(66, 32), TC_GREY, LOADER.GetImageN("io", 84), _("Next page"));
    // Hilfe
    AddImageButton(12, DrawPoint(16, GetSize().y - 47), Extent(32, 32), TC_GREY, LOADER.GetImageN("io", 225), _("Help"));

    waresPage.SetVisible(true);
}

void iwWares::Msg_ButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case 0: // "Blättern"
        {
            SetPage(curPage_ + 1);
        }
        break;
        case 12: // Hilfe
        {
            WINDOWMANAGER.ReplaceWindow(std::make_unique<iwHelp>(_("Here you will find a list of your entire stores of "
                                                                   "merchandise and all the inhabitants of your realm.")));
        }
        break;
    }
}

void iwWares::Msg_PaintBefore()
{
    IngameWindow::Msg_PaintBefore();

    // Farben ggf. aktualisieren

    if(curPage_ != pagePeople && curPage_ != pageWares)
        return;

    auto* group = GetCtrl<ctrlGroup>(100 + curPage_);
    if(group)
    {
        unsigned count = (curPage_ == pageWares) ? 36 : 32;

        for(unsigned i = 0; i < count; ++i)
        {
            auto* text = group->GetCtrl<ctrlVarText>(600 + i);
            if(text)
                text->SetTextColor((((curPage_ == pageWares) ? inventory.goods[i] : inventory.people[i]) == 0) ? COLOR_RED : COLOR_YELLOW);
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
    auto* group = GetCtrl<ctrlGroup>(100 + curPage_);
    if(group)
        group->SetVisible(false);

    // neue Page setzen
    curPage_ = page % numPages;

    // neue Page anzeigen
    group = GetCtrl<ctrlGroup>(100 + curPage_);
    if(group)
        group->SetVisible(true);
}

ctrlGroup& iwWares::AddPage()
{
    ctrlGroup& grp = *AddGroup(100 + numPages);
    numPages++;
    grp.SetVisible(false);
    return grp;
}
