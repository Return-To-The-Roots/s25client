// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
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

#include "dskCredits.h"
#include "Loader.h"
#include "WindowManager.h"
#include "controls/ctrlButton.h"
#include "drivers/VideoDriverWrapper.h"
#include "dskMainMenu.h"
#include "lua/GameDataLoader.h"
#include "ogl/FontStyle.h"
#include "ogl/MusicItem.h"
#include "ogl/glArchivItem_Bob.h"
#include "ogl/glFont.h"
#include "gameData/JobConsts.h"
#include "gameData/NationConsts.h"
#include "gameData/WorldDescription.h"
#include <array>
#include <cstdlib>
#include <numeric>

/** @class dskCredits
 *
 *  Klasse des Credits Desktops.
 */

/// Duration for one credits page
const unsigned PAGE_TIME = 12900;
/// Duration for fading between pages
const unsigned FADING_TIME = 2000;

dskCredits::dskCredits() : Desktop(LOADER.GetImageN("setup013", 0))
{
    // Zurück
    AddTextButton(0, DrawPoint(300, 550), Extent(200, 22), TC_RED1, _("Back"), NormalFont);

    AddText(1, DrawPoint(400, 10), _("Return To The Roots"), COLOR_YELLOW, FontStyle::CENTER, LargeFont);

    // "Credits"
    AddText(2, DrawPoint(400, 33), _("Credits"), COLOR_YELLOW, FontStyle::CENTER, LargeFont);

    CreditsEntry entry = CreditsEntry("Florian Doersch (FloSoft):", GetCreditsImgOrDefault("flosoft"));
    entry.lines.push_back(_("Project management"));
    entry.lines.push_back(_("Server management"));
    entry.lines.push_back(_("Programming"));
    entry.lines.push_back(_("Website Administration"));
    entry.lines.push_back(_("Website Programming"));
    entry.lines.push_back(_("Quality Assurance"));
    entries.push_back(entry);

    entry = CreditsEntry("Oliver Siebert (Oliverr):", GetCreditsImgOrDefault("oliverr"));
    entry.lines.push_back(_("Project management"));
    entry.lines.push_back(_("Programming"));
    entry.lines.push_back(_("Quality Assurance"));
    entries.push_back(entry);

    entry = CreditsEntry("Stefan Schüchl (Z-Stef):", GetCreditsImgOrDefault("z-stef"));
    entry.lines.push_back(_("Website Administration"));
    entry.lines.push_back(_("Website Programming"));
    entries.push_back(entry);

    entry = CreditsEntry("Patrick Haak (Demophobie):", GetCreditsImgOrDefault("demophobie"));
    entry.lines.push_back(_("Website Administration"));
    entry.lines.push_back(_("Quality Assurance"));
    entries.push_back(entry);

    entry = CreditsEntry("Jonas Trampe (NastX):", GetCreditsImgOrDefault("nastx"));
    entry.lines.push_back(_("Quality Assurance"));
    entry.lines.push_back(_("Mapping"));
    entries.push_back(entry);

    entry = CreditsEntry("Jan-Henrik Kluth (jh):", GetCreditsImgOrDefault("jh"));
    entry.lines.push_back(_("Programming"));
    entry.lines.push_back(_("Artificial Intelligence (AI)"));
    entries.push_back(entry);

    entry = CreditsEntry("Christopher Kuehnel (Spikeone):", GetCreditsImgOrDefault("spikeone"));
    entry.lines.push_back(_("Additional graphics"));
    entry.lines.push_back(_("Quality Assurance"));
    entry.lines.push_back(_("Mapping"));
    entries.push_back(entry);

    entry = CreditsEntry("Marcus Ströbel (Maqs):", GetCreditsImgOrDefault("maqs"));
    entry.lines.push_back(_("Programming"));
    entry.lines.push_back(_("Quality Assurance"));
    entries.push_back(entry);

    entry = CreditsEntry("Alex Grund (Flamefire):", GetCreditsImgOrDefault("flamefire"));
    entry.lines.push_back(_("Programming"));
    entry.lines.push_back(_("Quality Assurance"));
    entries.push_back(entry);

    entry = CreditsEntry(_("Additional Programming:"));
    entry.lines.push_back("Siegfried Oleg Pammer (siegi44)");
    entry.lines.push_back("Lienhart Woitok (liwo)");
    entry.lines.push_back("Christoph Erhardt (Airhardt)");
    entry.lines.push_back("Divan");
    entry.lines.push_back("Cat666");
    entry.lines.push_back("Devil");
    entry.lines.push_back("Ikhar Beq (PoC)");
    entries.push_back(entry);

    entry = CreditsEntry(_("Additional Graphics:"));
    entry.lines.push_back("Marcus Bullin (Parasit)");
    entries.push_back(entry);

    entry = CreditsEntry(_("Other Support:"));
    entry.lines.push_back("muhahahaha");
    entry.lines.push_back("Sotham");
    entry.lines.push_back("Fenan");
    entry.lines.push_back("Phil Groenewold (Phil333)");
    entry.lines.push_back("Marc Vester (xaser)");
    entries.push_back(entry);

    entry = CreditsEntry(_("Donators"), _("Thank you for your donations!"));
    entry.lines.push_back(_("various anonymous donators"));
    entry.lines.push_back("Markus Becker");
    entry.lines.push_back("Karsten Backhaus (K-Duke)");
    entry.lines.push_back("Patrick Haak (Demophobie)");
    entry.lines.push_back("Gilles Bordelais");
    entry.lines.push_back("Dominic Jonas");
    entry.lines.push_back("Rene Hopf");
    entry.lines.push_back("Christopher Kuehnel (Spikeone)");
    entry.lines.push_back("Philipp Strathausen");

    entry.lines.push_back(CreditsEntry::Line("Max Skuratov", 1));
    entry.lines.push_back(CreditsEntry::Line("Marius Loewe", 1));
    entry.lines.push_back(CreditsEntry::Line("Eric Lutter", 1));
    entry.lines.push_back(CreditsEntry::Line("Bob Kromonos Achten", 1));
    entry.lines.push_back(CreditsEntry::Line("morlock", 1));
    entry.lines.push_back(CreditsEntry::Line("Hans Gabathuler", 1));
    entry.lines.push_back(CreditsEntry::Line("Jan Montag", 1));
    entry.lines.push_back(CreditsEntry::Line("Patrick Schefczyk", 1));
    entries.push_back(entry);

    entry = CreditsEntry(_("We hope you enjoy playing Return To The Roots!"), _("THE END"));
    entry.lines.push_back(_("Thank you!"));
    entries.push_back(entry);

    WorldDescription worldDesc;
    GameDataLoader gdLoader(worldDesc);
    if(!gdLoader.Load())
        throw std::runtime_error("Failed to load game data");

    std::vector<Nation> nations(NUM_NATIVE_NATS);
    for(int i = 0; i < NUM_NATIVE_NATS; i++)
        nations[i] = Nation(i);

    LOADER.LoadFilesAtGame(worldDesc.get(DescIdx<LandscapeDesc>(0)).mapGfxPath, false, nations);

    this->itCurEntry = entries.begin();

    if(LOADER.sng_lst.size() > 8)
        LOADER.sng_lst[8]->Play(0);
}

dskCredits::~dskCredits() = default;

void dskCredits::Draw_()
{
    Desktop::Draw_();
    DrawBobs();
    DrawCredit();
}

void dskCredits::DrawCredit()
{
    unsigned time = VIDEODRIVER.GetTickCount() - startTime;
    if(time > PAGE_TIME)
    {
        // Next page
        ++this->itCurEntry;
        if(this->itCurEntry == entries.end())
            this->itCurEntry = entries.begin();
        startTime = VIDEODRIVER.GetTickCount();
    }

    // calculate text transparency
    unsigned transparency;
    if(time < FADING_TIME)
        transparency = 0xFF * time / FADING_TIME;
    else if(time >= PAGE_TIME)
        transparency = 0;
    else if(time > PAGE_TIME - FADING_TIME)
        transparency = (0xFF - 0xFF * (time - (PAGE_TIME - FADING_TIME)) / FADING_TIME);
    else
        transparency = 0xFF;

    // draw text
    LargeFont->Draw(DrawPoint(40, 100), itCurEntry->title, FontStyle{}, SetAlpha(COLOR_RED, transparency));

    std::array<unsigned, 2> columnToY = {{150, 150}};

    for(auto& line : itCurEntry->lines)
    {
        LargeFont->Draw(DrawPoint(60 + line.column * 350, columnToY[line.column]), line.line, FontStyle{},
                        SetAlpha(COLOR_YELLOW, transparency));
        columnToY[line.column] += LargeFont->getHeight() + 5;
    }

    LargeFont->Draw(DrawPoint(40, columnToY[0] + 20), itCurEntry->lastLine, FontStyle{}, SetAlpha(COLOR_RED, transparency));

    if(itCurEntry->pic)
        itCurEntry->pic->DrawFull(DrawPoint(VIDEODRIVER.GetRenderSize().x - 300, 70), SetAlpha(COLOR_WHITE, transparency));
}

void dskCredits::DrawBobs()
{
    // Frameratebegrenzer
    int msSinceLastBobAnim = VIDEODRIVER.GetTickCount() - bobTime;
    int bobAnimStepsPerSec = 25;

    int msSinceLastBobSpawn = VIDEODRIVER.GetTickCount() - bobSpawnTime;
    int bob_spawnprosec = 5;

    if(VIDEODRIVER.GetFPS() < 30)
        bob_spawnprosec = 0;
    else if(VIDEODRIVER.GetFPS() < 60)
        bob_spawnprosec = 1;
    else if(VIDEODRIVER.GetFPS() < 200)
        bob_spawnprosec = 2;

    // add new bob
    if(bob_spawnprosec > 0 && msSinceLastBobSpawn > (1000 / bob_spawnprosec)
       && (int)bobs.size() < (int)(50 + VIDEODRIVER.GetRenderSize().x / 2))
    {
        bobSpawnTime = VIDEODRIVER.GetTickCount();

        Bob b = Bob();
        b.animationStep = 0;
        b.speed = 1 + rand() % 4;

        // links oder rechts spawnen
        if(rand() % 2 == 0)
        {
            b.pos.x = 0;
            b.direction = libsiedler2::ImgDir::E;
        } else
        {
            b.pos.x = VIDEODRIVER.GetRenderSize().x;
            b.direction = libsiedler2::ImgDir::W;
        }

        b.color = PLAYER_COLORS[rand() % PLAYER_COLORS.size()];
        unsigned job = rand() % 29;

        // exclude "headless" bobs
        if(job == 8 || job == 9 || job == 12 || job == 18)
        {
            job = rand() % (NUM_WARE_TYPES - 1);
            b.hasWare = true;
        } else
        {
            job = JOB_SPRITE_CONSTS[job].getBobId(Nation(rand() % NUM_NATS));
            b.hasWare = false;
        }

        b.id = job;
        b.pos.y = GetCtrl<ctrlButton>(0)->GetPos().y - 20 - rand() % 150;
        bobs.push_back(b);
    }

    // draw bobs
    for(auto& bob : bobs)
    {
        if(!bob.hasWare)
            LOADER.GetBob("jobs")->Draw(bob.id, bob.direction, bob.isFat, bob.animationStep, bob.pos, bob.color);
        else
            LOADER.GetBob("carrier")->Draw(bob.id, bob.direction, bob.isFat, bob.animationStep, bob.pos, bob.color);

        if(msSinceLastBobAnim > (1000 / bobAnimStepsPerSec))
        {
            bobTime = VIDEODRIVER.GetTickCount();

            bob.animationStep++;
            if(bob.animationStep > 7)
                bob.animationStep = 0;
            if(bob.direction == libsiedler2::ImgDir::E)
            {
                bob.pos.x += bob.speed;
                if(bob.pos.x > static_cast<int>(VIDEODRIVER.GetRenderSize().x))
                    bob.direction = libsiedler2::ImgDir::W;
            } else if(bob.direction == libsiedler2::ImgDir::W)
            {
                bob.pos.x -= bob.speed;
                if(bob.pos.x < 0)
                    bob.direction = libsiedler2::ImgDir::E;
            }
        }
    }

    // Frameratebegrenzer aktualisieren
    if(msSinceLastBobAnim > (1000 / bobAnimStepsPerSec))
        bobTime = VIDEODRIVER.GetTickCount();
}

bool dskCredits::Close()
{
    WINDOWMANAGER.Switch(std::make_unique<dskMainMenu>());
    return true;
}

glArchivItem_Bitmap* dskCredits::GetCreditsImgOrDefault(const std::string& name)
{
    glArchivItem_Bitmap* result = LOADER.GetImage("credits", name);
    if(!result)
        result = LOADER.GetImage("credits", "default");
    return result;
}

bool dskCredits::Msg_KeyDown(const KeyEvent& /*ke*/)
{
    return Close();
}

void dskCredits::Msg_ButtonClick(const unsigned /*ctrl_id*/)
{
    Close();
}

void dskCredits::SetActive(bool active)
{
    Desktop::SetActive(active);
    if(active)
    {
        startTime = bobTime = bobSpawnTime = VIDEODRIVER.GetTickCount();
    }
}
