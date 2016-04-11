// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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

///////////////////////////////////////////////////////////////////////////////
// Header
#include "defines.h" // IWYU pragma: keep
#include "dskCredits.h"

#include "GameManager.h"
#include "WindowManager.h"
#include "Loader.h"

#include "dskMainMenu.h"
#include "controls/ctrlButton.h"

#include "drivers/VideoDriverWrapper.h"
#include "gameData/JobConsts.h"
#include "ogl/glArchivItem_Font.h"
#include "ogl/glArchivItem_Music.h"
#include "ogl/glArchivItem_Bob.h"
#include <boost/array.hpp>
#include <cstdlib>

// Include last!
#include "DebugNew.h" // IWYU pragma: keep
struct KeyEvent;
///////////////////////////////////////////////////////////////////////////////
/** @class dskCredits
 *
 *  Klasse des Credits Desktops.
 *
 *  @author FloSoft
 */


/// Duration for one credits page
const unsigned PAGE_TIME = 12900;
/// Duration for fading between pages
const unsigned FADING_TIME = 2000;

dskCredits::dskCredits() : Desktop(LOADER.GetImageN("setup013", 0))
{
    // Zurück
    AddTextButton(0, 300, 550, 200, 22,   TC_RED1, _("Back"), NormalFont);

    // "Die Siedler II.5 RTTR"
    AddText(1, 400, 10, _("Return To The Roots"), COLOR_YELLOW, glArchivItem_Font::DF_CENTER, LargeFont);

    // "Credits"
    AddText(2, 400, 33, _("Credits"), COLOR_YELLOW, glArchivItem_Font::DF_CENTER, LargeFont);

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

    bool nations[NAT_COUNT] = { true, true, true, true, false };

    LOADER.LoadFilesAtGame(0, nations);

    this->itCurEntry = entries.begin();
    startTime = bobTime = bobSpawnTime = VIDEODRIVER.GetTickCount();

    if(GetMusic(sng_lst, 8))
        GetMusic(sng_lst, 8)->Play(0);
}

dskCredits::~dskCredits()
{
}

void dskCredits::Msg_PaintAfter()
{
    unsigned int time = VIDEODRIVER.GetTickCount() - startTime;

    if (time > PAGE_TIME)
    {
        ++this->itCurEntry;
        if (this->itCurEntry == entries.end())
            this->itCurEntry = entries.begin();
        this->startTime = VIDEODRIVER.GetTickCount();
    }

    // Frameratebegrenzer
    int bob_time = VIDEODRIVER.GetTickCount() - bobTime;
    int bob_prosec = 25;

    int bob_spawntime = VIDEODRIVER.GetTickCount() - bobSpawnTime;
    int bob_spawnprosec = 5;

    if(GAMEMANAGER.GetFPS() < 30)
        bob_spawnprosec = 0;
    else if(GAMEMANAGER.GetFPS() < 60)
        bob_spawnprosec = 1;
    else if(GAMEMANAGER.GetFPS() < 200)
        bob_spawnprosec = 2;

    // add new bob
    if ( bob_spawnprosec > 0 && bob_spawntime > (1000 / bob_spawnprosec) && (int)bobs.size() < (int)(50 + VIDEODRIVER.GetScreenWidth() / 2))
    {
        bobSpawnTime = VIDEODRIVER.GetTickCount();

        Bob b = Bob();
        b.animationStep = 0;
        b.speed = 1 + rand() % 4;

        // links oder rechts spawnen
        if(rand() % 2 == 0)
        {
            b.x = 0;
            b.direction = 3;
        }
        else
        {
            b.x = VIDEODRIVER.GetScreenWidth();
            b.direction = 6;
        }

        b.color = PLAYER_COLORS[rand() % PLAYER_COLORS.size()];
        unsigned int job = rand() % 29;

        // exclude "headless" bobs
        if (job == 8 || job == 9 || job == 12 || job == 18)
        {
            job = rand() % (WARE_TYPES_COUNT - 1);
            b.hasWare = true;
        }
        else
        {
            if(job == JOB_SCOUT)
                job = 35 + NATION_RTTR_TO_S2[rand() % 4] * 6;
            else if(job >= JOB_PRIVATE && job <= JOB_GENERAL)
                job = 30 + NATION_RTTR_TO_S2[rand() % 4] * 6 + job - JOB_PRIVATE;
            else
                job = JOB_CONSTS[job].jobs_bob_id;
            b.hasWare = false;
        }

        b.id = job;
        b.y = GetCtrl<ctrlButton>(0)->GetY() - 20 - rand() % 150;
        bobs.push_back(b);
    }

    // draw bobs
    for (std::vector<Bob>::iterator bob = bobs.begin(); bob != bobs.end(); ++bob)
    {
        if (!bob->hasWare)
            LOADER.GetBobN("jobs")->Draw(bob->id, bob->direction, bob->isFat, bob->animationStep, bob->x, bob->y, bob->color);
        else
            LOADER.GetBobN("carrier")->Draw(bob->id, bob->direction, bob->isFat, bob->animationStep, bob->x, bob->y, bob->color);

        if( bob_time > (1000 / bob_prosec) )
        {
            bobTime = VIDEODRIVER.GetTickCount();

            bob->animationStep++;
            if (bob->animationStep > 7)
                bob->animationStep = 0;
            if (bob->direction == 3)
            {
                bob->x += bob->speed;
                if (bob->x > VIDEODRIVER.GetScreenWidth())
                    bob->direction = 6;
            }
            else if (bob->direction == 6)
            {
                bob->x -= bob->speed;
                if (bob->x < 0)
                    bob->direction = 3;
            }
        }
    }

    // Frameratebegrenzer aktualisieren
    if( bob_time > (1000 / bob_prosec) )
        bobTime = VIDEODRIVER.GetTickCount();

    // calculate text transparency
    unsigned transparency = 0xFF;

    if(time < FADING_TIME)
        transparency = 0xFF * time / FADING_TIME;
    if (time > PAGE_TIME - FADING_TIME)
        transparency = (0xFF - 0xFF * (time - (PAGE_TIME - FADING_TIME)) / FADING_TIME);

    if (time > PAGE_TIME)
        transparency = 0;

    transparency = transparency << 24;

    // draw text
    LargeFont->Draw(40, 100, itCurEntry->title, 0, (COLOR_RED & 0x00FFFFFF) | transparency);

    boost::array<unsigned int, 2> columnToY = {{150, 150}};

    for(std::vector<CreditsEntry::Line>::iterator line = itCurEntry->lines.begin(); line != itCurEntry->lines.end(); ++line)
    {
        LargeFont->Draw(60 + line->column * 350, columnToY[line->column], line->line, 0, (COLOR_YELLOW & 0x00FFFFFF) | transparency);
        columnToY[line->column] += LargeFont->getHeight() + 5;
    }

    LargeFont->Draw(40, columnToY[0] + 20, itCurEntry->lastLine, 0, (COLOR_RED & 0x00FFFFFF) | transparency);

    if (itCurEntry->pic)
        itCurEntry->pic->Draw(VIDEODRIVER.GetScreenWidth() - 300, 70, 0, 0, 0, 0, 0, 0, (COLOR_WHITE & 0x00FFFFFF) | transparency);
}

bool dskCredits::Close()
{
    WINDOWMANAGER.Switch(new dskMainMenu());
    return true;
}

glArchivItem_Bitmap* dskCredits::GetCreditsImgOrDefault(const std::string& name)
{
    glArchivItem_Bitmap* result = LOADER.GetImage("credits", name + ".bmp");
    if(!result)
        result = LOADER.GetImage("credits", "default.bmp");
    return result;
}

bool dskCredits::Msg_KeyDown(const KeyEvent&  /*ke*/)
{
    return Close();
}


void dskCredits::Msg_ButtonClick(const unsigned  /*ctrl_id*/)
{
    Close();
}

///////////////////////////////////////////////////////////////////////////////
// EOF
///////////////////////////////////////////////////////////////////////////////

