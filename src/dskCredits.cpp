// $Id: dskCredits.cpp 9357 2014-04-25 15:35:25Z FloSoft $
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
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
#include "main.h"
#include "dskCredits.h"

#include "GameManager.h"
#include "WindowManager.h"
#include "Loader.h"

#include "dskMainMenu.h"
#include "ctrlButton.h"

#include "VideoDriverWrapper.h"
#include "JobConsts.h"

#include <cstdlib>
#include <ctime>

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

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

///////////////////////////////////////////////////////////////////////////////
/**
 *  Konstruktor von @p dskCredits.
 *
 *  @author siegi44
 */
dskCredits::dskCredits(void) : Desktop(LOADER.GetImageN("setup013", 0))
{
    // Zurück
    AddTextButton(0, 300, 550, 200, 22,   TC_RED1, _("Back"), NormalFont);

    // "Die Siedler II.5 RTTR"
    AddText(1, 400, 10, _("Return To The Roots"), COLOR_YELLOW, glArchivItem_Font::DF_CENTER, LargeFont);

    // "Credits"
    AddText(2, 400, 33, _("Credits"), COLOR_YELLOW, glArchivItem_Font::DF_CENTER, LargeFont);

    CreditsEntry entry = CreditsEntry();
    entry.title = "Florian Doersch (FloSoft):";
    entry.picId = 1;
    entry.lastLine = "";
    entry.lines.push_back(CreditsEntry::Line(_("Project management")));
    entry.lines.push_back(CreditsEntry::Line(_("Server management")));
    entry.lines.push_back(CreditsEntry::Line(_("Programming")));
    entry.lines.push_back(CreditsEntry::Line(_("Website Administration")));
    entry.lines.push_back(CreditsEntry::Line(_("Website Programming")));
    entry.lines.push_back(CreditsEntry::Line(_("Quality Assurance")));

    this->entries.push_back(entry);
    entry.lines.clear();

    entry.title = "Oliver Siebert (Oliverr):";
    entry.picId = 4;
    entry.lastLine = "";
    entry.lines.push_back(CreditsEntry::Line(_("Project management")));
    entry.lines.push_back(CreditsEntry::Line(_("Programming")));
    entry.lines.push_back(CreditsEntry::Line(_("Quality Assurance")));

    this->entries.push_back(entry);
    entry.lines.clear();

    entry.title = "Stefan Schüchl (Z-Stef):";
    entry.picId = 6;
    entry.lastLine = "";
    entry.lines.push_back(CreditsEntry::Line(_("Website Administration")));
    entry.lines.push_back(CreditsEntry::Line(_("Website Programming")));

    this->entries.push_back(entry);
    entry.lines.clear();

    entry.title = "Patrick Haak (Demophobie):";
    entry.picId = 0;
    entry.lastLine = "";
    entry.lines.push_back(CreditsEntry::Line(_("Website Administration")));
    entry.lines.push_back(CreditsEntry::Line(_("Quality Assurance")));

    this->entries.push_back(entry);
    entry.lines.clear();

    entry.title = "Jonas Trampe (NastX):";
    entry.picId = 3;
    entry.lastLine = "";
    entry.lines.push_back(CreditsEntry::Line(_("Quality Assurance")));
    entry.lines.push_back(CreditsEntry::Line(_("Mapping")));

    this->entries.push_back(entry);
    entry.lines.clear();

    entry.title = "Jan-Henrik Kluth (jh):";
    entry.picId = 2;
    entry.lastLine = "";
    entry.lines.push_back(CreditsEntry::Line(_("Programming")));
    entry.lines.push_back(CreditsEntry::Line(_("Artificial Intelligence (AI)")));

    this->entries.push_back(entry);
    entry.lines.clear();

    entry.title = "Christopher Kuehnel (Spikeone):";
    entry.picId = 5;
    entry.lastLine = "";
    entry.lines.push_back(CreditsEntry::Line(_("Additional graphics")));
    entry.lines.push_back(CreditsEntry::Line(_("Quality Assurance")));
    entry.lines.push_back(CreditsEntry::Line(_("Mapping")));

    this->entries.push_back(entry);
    entry.lines.clear();

    entry.title = "Marcus Ströbel (Maqs):";
    entry.picId = -1;
    entry.lastLine = "";
    entry.lines.push_back(CreditsEntry::Line(_("Programming")));
    entry.lines.push_back(CreditsEntry::Line(_("Quality Assurance")));

    this->entries.push_back(entry);
    entry.lines.clear();

    entry.title = _("Additional Programming:");
    entry.picId = -1;
    entry.lastLine = "";
    entry.lines.push_back(CreditsEntry::Line("Siegfried Oleg Pammer (siegi44)"));
    entry.lines.push_back(CreditsEntry::Line("Lienhart Woitok (liwo)"));
    entry.lines.push_back(CreditsEntry::Line("Christoph Erhardt (Airhardt)"));
    entry.lines.push_back(CreditsEntry::Line("Divan"));
    entry.lines.push_back(CreditsEntry::Line("Cat666"));
    entry.lines.push_back(CreditsEntry::Line("Devil"));
    entry.lines.push_back(CreditsEntry::Line("Ikhar Beq (PoC)"));

    this->entries.push_back(entry);
    entry.lines.clear();

    entry.title = _("Additional Graphics:");
    entry.picId = -1;
    entry.lastLine = "";
    entry.lines.push_back(CreditsEntry::Line("Marcus Bullin (Parasit)"));

    this->entries.push_back(entry);
    entry.lines.clear();

    entry.title = _("Other Support:");
    entry.picId = -1;
    entry.lastLine = "";
    entry.lines.push_back(CreditsEntry::Line("muhahahaha"));
    entry.lines.push_back(CreditsEntry::Line("Sotham"));
    entry.lines.push_back(CreditsEntry::Line("Fenan"));
    entry.lines.push_back(CreditsEntry::Line("Phil Groenewold (Phil333)"));
    entry.lines.push_back(CreditsEntry::Line("Marc Vester (xaser)"));

    this->entries.push_back(entry);
    entry.lines.clear();

    entry.title = _("Donators");
    entry.picId = -1;
    entry.lines.push_back(CreditsEntry::Line(_("various anonymous donators")));
    entry.lines.push_back(CreditsEntry::Line("Markus Becker"));
    entry.lines.push_back(CreditsEntry::Line("Karsten Backhaus (K-Duke)"));
    entry.lines.push_back(CreditsEntry::Line("Patrick Haak (Demophobie)"));
    entry.lines.push_back(CreditsEntry::Line("Gilles Bordelais"));
    entry.lines.push_back(CreditsEntry::Line("Dominic Jonas"));
    entry.lines.push_back(CreditsEntry::Line("Rene Hopf"));
    entry.lines.push_back(CreditsEntry::Line("Christopher Kuehnel (Spikeone)"));
    entry.lines.push_back(CreditsEntry::Line("Philipp Strathausen"));

    entry.lines.push_back(CreditsEntry::Line("Max Skuratov", 1));
    entry.lines.push_back(CreditsEntry::Line("Marius Loewe", 1));
    entry.lines.push_back(CreditsEntry::Line("Eric Lutter", 1));
    entry.lines.push_back(CreditsEntry::Line("Bob Kromonos Achten", 1));
    entry.lines.push_back(CreditsEntry::Line("morlock", 1));
    entry.lines.push_back(CreditsEntry::Line("Hans Gabathuler", 1));
    entry.lines.push_back(CreditsEntry::Line("Jan Montag", 1));
    entry.lines.push_back(CreditsEntry::Line("Patrick Schefczyk", 1));
    entry.lastLine = _("Thank you for your donations!");

    this->entries.push_back(entry);
    entry.lines.clear();

    entry.title = _("We hope you enjoy playing Return To The Roots!");
    entry.picId = -1;
    entry.lines.push_back(CreditsEntry::Line(_("Thank you!")));
    entry.lastLine = _("THE END");

    this->entries.push_back(entry);
    entry.lines.clear();

    bool nations[NATION_COUNT] = { true, true, true, true, false };

    LOADER.LoadFilesAtGame(0, nations);

    this->it = entries.begin();
    startTime = bobTime = bobSpawnTime = VideoDriverWrapper::inst().GetTickCount();

    GetMusic(sng_lst, 8)->Play(0);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author FloSoft
 */
dskCredits::~dskCredits()
{
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author siegi44
 */
void dskCredits::Msg_PaintAfter()
{
    unsigned int time = VideoDriverWrapper::inst().GetTickCount() - startTime;

    if (time > PAGE_TIME)
    {
        ++this->it;
        if (this->it == entries.end())
            this->it = entries.begin();
        this->startTime = VideoDriverWrapper::inst().GetTickCount();
    }

    // Frameratebegrenzer
    int bob_time = VideoDriverWrapper::inst().GetTickCount() - bobTime;
    int bob_prosec = 25;

    int bob_spawntime = VideoDriverWrapper::inst().GetTickCount() - bobSpawnTime;
    int bob_spawnprosec = 5;

    if(GAMEMANAGER.GetFPS() < 30)
        bob_spawnprosec = 0;
    else if(GAMEMANAGER.GetFPS() < 60)
        bob_spawnprosec = 1;
    else if(GAMEMANAGER.GetFPS() < 200)
        bob_spawnprosec = 2;

    // add new bob
    if ( bob_spawnprosec > 0 && bob_spawntime > (1000 / bob_spawnprosec) && (int)bobs.size() < (int)(50 + VideoDriverWrapper::inst().GetScreenWidth() / 2))
    {
        bobSpawnTime = VideoDriverWrapper::inst().GetTickCount();

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
            b.x = VideoDriverWrapper::inst().GetScreenWidth();
            b.direction = 6;
        }

        b.color = COLORS[rand() % PLAYER_COLORS_COUNT];
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
    for (std::list<Bob>::iterator bob = bobs.begin(); bob != bobs.end(); ++bob)
    {
        if (!bob->hasWare)
            Loader::inst().GetBobN("jobs")->Draw(bob->id, bob->direction, bob->isFat, bob->animationStep, bob->x, bob->y, bob->color);
        else
            Loader::inst().GetBobN("carrier")->Draw(bob->id, bob->direction, bob->isFat, bob->animationStep, bob->x, bob->y, bob->color);

        if( bob_time > (1000 / bob_prosec) )
        {
            bobTime = VideoDriverWrapper::inst().GetTickCount();

            bob->animationStep++;
            if (bob->animationStep > 7)
                bob->animationStep = 0;
            if (bob->direction == 3)
            {
                bob->x += bob->speed;
                if (bob->x > VideoDriverWrapper::inst().GetScreenWidth())
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
        bobTime = VideoDriverWrapper::inst().GetTickCount();

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
    LargeFont->Draw(40, 100, it->title, 0, (COLOR_RED & 0x00FFFFFF) | transparency);

    unsigned int y[2] = {150, 150};

    for(std::list<CreditsEntry::Line>::iterator line = this->it->lines.begin(); line != it->lines.end(); ++line)
    {
        LargeFont->Draw(60 + line->column * 350, y[line->column], line->line.c_str(), 0, (COLOR_YELLOW & 0x00FFFFFF) | transparency);
        y[line->column] += LargeFont->getHeight() + 5;
    }

    LargeFont->Draw(40, y[0] + 20, it->lastLine, 0, (COLOR_RED & 0x00FFFFFF) | transparency);

    // draw picture
    glArchivItem_Bitmap* item = LOADER.GetImageN("credits", it->picId);

    if (item)
        item->Draw(VideoDriverWrapper::inst().GetScreenWidth() - 300, 70, 0, 0, 0, 0, 0, 0, (COLOR_WHITE & 0x00FFFFFF) | transparency);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author siegi44
 */
bool dskCredits::Close(void)
{
    WindowManager::inst().Switch(new dskMainMenu());
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author siegi44
 */
bool dskCredits::Msg_KeyDown(const KeyEvent& ke)
{
    return Close();
}


void dskCredits::Msg_ButtonClick(const unsigned ctrl_id)
{
    Close();
}

///////////////////////////////////////////////////////////////////////////////
// EOF
///////////////////////////////////////////////////////////////////////////////

