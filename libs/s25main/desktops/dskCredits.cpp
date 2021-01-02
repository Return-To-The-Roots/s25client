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
#include "controls/ctrlTimer.h"
#include "drivers/VideoDriverWrapper.h"
#include "dskMainMenu.h"
#include "helpers/mathFuncs.h"
#include "ingameWindows/iwMsgbox.h"
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

using namespace std::chrono_literals;
/// Duration for one credits page
const auto PAGE_TIME = 12900ms;
/// Duration for fading between pages
const auto FADING_TIME = 2s;
using std::chrono::duration_cast;

namespace {
enum
{
    ID_btBack,
    ID_txtRttr,
    ID_txtCredits,
    ID_tmrBobAnim,
    ID_tmrPage,
};
}

dskCredits::dskCredits() : Desktop(LOADER.GetImageN("setup013", 0)), itCurEntry(entries.end())
{
    AddTextButton(ID_btBack, DrawPoint(300, 550), Extent(200, 22), TC_RED1, _("Back"), NormalFont);
    AddText(ID_txtRttr, DrawPoint(400, 10), _("Return To The Roots"), COLOR_YELLOW, FontStyle::CENTER, LargeFont);
    AddText(ID_txtCredits, DrawPoint(400, 33), _("Credits"), COLOR_YELLOW, FontStyle::CENTER, LargeFont);
    AddTimer(ID_tmrBobAnim, 40ms);
    pageTimer = AddTimer(ID_tmrPage, PAGE_TIME);
    pageTimer->Stop();

    LOADER.Load(ResourceId("credits")); // Ignore failure

    // order by last name (alphabetical)
    CreditsEntry entry = CreditsEntry("Alexander Grund (Flamefire):", GetCreditsImgOrDefault("flamefire"));
    entry.lines.push_back(_("Programming"));
    entry.lines.push_back(_("Quality Assurance"));
    entries.push_back(entry);

    entry = CreditsEntry("Patrick Haak (Demophobie):", GetCreditsImgOrDefault("demophobie"));
    entry.lines.push_back(_("Website Administration"));
    entry.lines.push_back(_("Quality Assurance"));
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

    entry = CreditsEntry("Stefan Schüchl (Z-Stef):", GetCreditsImgOrDefault("z-stef"));
    entry.lines.push_back(_("Website Administration"));
    entry.lines.push_back(_("Website Programming"));
    entries.push_back(entry);

    entry = CreditsEntry("Marcus Ströbel (Maqs):", GetCreditsImgOrDefault("maqs"));
    entry.lines.push_back(_("Programming"));
    entry.lines.push_back(_("Quality Assurance"));
    entries.push_back(entry);

    entry = CreditsEntry("Jonas Trampe (NastX):", GetCreditsImgOrDefault("nastx"));
    entry.lines.push_back(_("Quality Assurance"));
    entry.lines.push_back(_("Mapping"));
    entries.push_back(entry);

    // founder members
    entry = CreditsEntry("Oliver Siebert (Oliverr):", GetCreditsImgOrDefault("oliverr"));
    entry.lines.push_back(_("Project founder"));
    entry.lines.push_back(_("Project management"));
    entry.lines.push_back(_("Programming"));
    entry.lines.push_back(_("Quality Assurance"));
    entries.push_back(entry);

    entry = CreditsEntry("Florian Doersch (FloSoft):", GetCreditsImgOrDefault("flosoft"));
    entry.lines.push_back(_("Project founder"));
    entry.lines.push_back(_("Project management"));
    entry.lines.push_back(_("Server management"));
    entry.lines.push_back(_("Programming"));
    entry.lines.push_back(_("Website Administration"));
    entry.lines.push_back(_("Website Programming"));
    entry.lines.push_back(_("Quality Assurance"));
    entry.lines.push_back(_("Finances"));
    entries.push_back(entry);

    // alphabetical again
    entry = CreditsEntry(_("Additional Programming:"));
    entry.lines.push_back("Ikhar Beq (PoC)");
    entry.lines.push_back("Cat666");
    entry.lines.push_back("Devil");
    entry.lines.push_back("Divan");
    entry.lines.push_back("Christoph Erhardt (Airhardt)");
    entry.lines.push_back("Siegfried Oleg Pammer (siegi44)");
    entry.lines.push_back("Jonathan Steinbuch");
    entry.lines.push_back("Lienhart Woitok (liwo)");
    entry.lines.push_back("");
    entry.lines.push_back(_("all developers who contributed via Github"));
    entries.push_back(entry);

    // alphabetical again
    entry = CreditsEntry(_("Additional Graphics:"));
    entry.lines.push_back("Marcus Bullin (Parasit)");
    entries.push_back(entry);

    // alphabetical again
    entry = CreditsEntry(_("Additional Support:"));
    entry.lines.push_back("Fenan");
    entry.lines.push_back("Phil Groenewold (Phil333)");
    entry.lines.push_back("muhahahaha");
    entry.lines.push_back("Sotham");
    entry.lines.push_back("Marc Vester (xaser)");
    entries.push_back(entry);

    // add all donators to this list between first/last entry
    const std::vector<std::string> donators = {
      _("various anonymous donators"),
      // A
      "Bob Kromonos Achten", //
      "Alles Adam",          //
      "Niklas Anders",       //
      "Christian Arpe",      //
      // B
      "Karsten Backhaus (K-Duke)", //
      "Günter Baumann",            //
      "Felix Becker",              //
      "Markus Becker",             //
      "Sebastian Bernhard",        //
      "Gilles Bordelais",          //
      "André Brem",                //
      "Hannes Brüske",             //
      "Andreas Brüske",            //
      // C
      // D
      "Jannes Dirks", //
      // E
      "Gerrit Eberhardt", //
      // F
      "Alexander Faber",          //
      "Niklas Faig",              //
      "Christopher Flocke",       //
      "Christopher Funke-Kaiser", //
      // G
      "Hans Gabathuler", //
      "Thomas Georg",    //
      "Konrad Greinke",  //
      "Stefan Gunkel",   //
      // H
      "Patrick Haak (Demophobie)", //
      "Marius Hacker",             //
      "Daniel Hampf",              //
      "Nathan Hall",               //
      "Christoph Hartmann",        //
      "Andreas Hauer",             //
      "Stephan Hesse",             //
      "Daniel Holle",              //
      "Rene Hopf",                 //
      "Hanso Hunder",              //
      "Benjamin Hünig",            //
      // I
      // J
      "Dominic Jonas", //
      "Simon Jais",    //
      // K
      "Silvio Karbe",                   //
      "Ralli Kasikas",                  //
      "Jörg Kesten",                    //
      "Thorsten Kindel",                //
      "Holger Klötzner",                //
      "Andreas Kniep",                  //
      "Vladislav Kolaja",               //
      "Daniel Krsiak",                  //
      "Andreas Krimm",                  //
      "Christopher Kuehnel (Spikeone)", //
      // L
      "Alexander Lambio", //
      "Oliver Lang",      //
      "Marius Loewe",     //
      "Eric Lutter",      //
      // M
      "Jan Montag",  //
      "Kai Müller",  //
      "morlock",     //
      "Jan Mozgawa", //
      // N
      // O
      // P
      "Wojciech Pieklik", //
      "Mike Plackowski",  //
      "Daniel Platt",     //
      // Q
      // R
      "Philip Rau",    //
      "Ronny Richter", //
      // S
      "Daniel Seindl",       //
      "Vasilyev Sergey",     //
      "Patrick Schefczyk",   //
      "Marcel Schneider",    //
      "Alexander Schoedon",  //
      "Max Skuratov",        //
      "Philipp Strathausen", //
      "Benjamin Stoisch",    //
      "Felix Stolle",        //
      // T
      "Nina Tillmann", //
      "Angelo Tiegs",  //
      // U
      // V
      // W
      "Niels Wiederanders", //
      "Philipp Wohlleben",  //
      // X
      // Y
      // Z
      _("... and many more") //
    };

    // alphabetical again, two columns
    entry = CreditsEntry(_("Donators"), _("Thank you for your donations!"));

    const size_t page_size = 15;
    const size_t middle = std::min(page_size, donators.size()) / 2;
    size_t pos = 0;
    for(const auto& donator : donators)
    {
        const size_t column = (pos < middle ? 0 : 1);
        entry.lines.push_back(CreditsEntry::Line(donator, column));
        ++pos;

        // create new page
        if(pos > page_size)
        {
            entries.push_back(entry);
            entry.lines.clear();
            pos = 0;
        }
    }

    if(!entry.lines.empty())
    {
        entries.push_back(entry);
    }

    entry = CreditsEntry(_("We hope you enjoy playing Return To The Roots!"), _("THE END"));
    entry.lines.push_back(_("Thank you!"));
    entries.push_back(entry);

    WorldDescription worldDesc;
    GameDataLoader gdLoader(worldDesc);
    if(!gdLoader.Load())
    {
        WINDOWMANAGER.Show(std::make_unique<iwMsgbox>(_("Error"), _("Failed to load game data"), this, MsgboxButton::Ok,
                                                      MsgboxIcon::ExclamationRed, 0));
        return;
    }

    std::vector<Nation> nations(NUM_NATIVE_NATIONS);
    for(unsigned i = 0; i < NUM_NATIVE_NATIONS; i++)
        nations[i] = Nation(i);

    if(!LOADER.LoadFilesAtGame(worldDesc.get(DescIdx<LandscapeDesc>(0)).mapGfxPath, false, nations, {}))
    {
        WINDOWMANAGER.Show(std::make_unique<iwMsgbox>(_("Error"), _("Failed to load game resources"), this,
                                                      MsgboxButton::Ok, MsgboxIcon::ExclamationRed, 0));
        return;
    }

    this->itCurEntry = entries.begin();

    if(LOADER.sng_lst.size() > 8)
        LOADER.sng_lst[8]->Play(-1);
}

dskCredits::~dskCredits() = default;

void dskCredits::Draw_()
{
    Desktop::Draw_();
    if(itCurEntry != entries.end())
    {
        DrawBobs();
        DrawCredit();
    }
}

void dskCredits::GotoNextPage()
{
    ++itCurEntry;
    if(itCurEntry == entries.end())
        itCurEntry = entries.begin();
    pageTimer->Start();
}

void dskCredits::GotoPrevPage()
{
    if(itCurEntry == entries.begin())
        itCurEntry = std::prev(entries.end());
    else
        --itCurEntry;
    pageTimer->Start();
}

void dskCredits::DrawCredit()
{
    const auto elapsedPageTime = std::chrono::duration_cast<std::chrono::milliseconds>(pageTimer->getElapsed());

    // calculate text transparency
    const auto fadeoutStartTime = PAGE_TIME - FADING_TIME;
    const unsigned transparency = (elapsedPageTime >= fadeoutStartTime) ?
                                    helpers::interpolate(0xFFu, 0u, elapsedPageTime - fadeoutStartTime, FADING_TIME) :
                                    helpers::interpolate(0u, 0xFFu, elapsedPageTime, FADING_TIME);

    // draw text
    LargeFont->Draw(DrawPoint(40, 100), itCurEntry->title, FontStyle{}, SetAlpha(COLOR_RED, transparency));

    std::array<unsigned, 2> columnToY = {{150, 150}};

    for(const auto& line : itCurEntry->lines)
    {
        LargeFont->Draw(DrawPoint(60 + line.column * 350, columnToY[line.column]), line.line, FontStyle{},
                        SetAlpha(COLOR_YELLOW, transparency));
        columnToY[line.column] += LargeFont->getHeight() + 5;
    }

    LargeFont->Draw(DrawPoint(40, columnToY[0] + 20), itCurEntry->lastLine, FontStyle{},
                    SetAlpha(COLOR_RED, transparency));

    if(itCurEntry->pic)
        itCurEntry->pic->DrawFull(DrawPoint(VIDEODRIVER.GetRenderSize().x - 300, 70),
                                  SetAlpha(COLOR_WHITE, transparency));
}

template<typename T>
T randEnum()
{
    return T(rand() % helpers::NumEnumValues_v<T>);
}

void dskCredits::DrawBobs()
{
    using namespace std::chrono_literals;
    auto bobSpawnInterval = 200ms;

    if(VIDEODRIVER.GetFPS() < 30)
        bobSpawnInterval = 0ms;
    else if(VIDEODRIVER.GetFPS() < 60)
        bobSpawnInterval = 1s;
    else if(VIDEODRIVER.GetFPS() < 200)
        bobSpawnInterval = 500ms;

    const auto maxNumBos = 50 + VIDEODRIVER.GetRenderSize().x / 2;

    // add new bob
    if(bobSpawnInterval != 0ms && bobSpawnTimer.getElapsed() >= bobSpawnInterval && bobs.size() < maxNumBos)
    {
        bobSpawnTimer.restart();

        Bob b = Bob();
        b.animationStep = 0;
        b.speed = 1 + rand() % 4;

        // spawn left or right
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
        const auto job = randEnum<Job>();

        // exclude "headless" bobs
        if(job == Job::Miller || job == Job::Baker || job == Job::Brewer || job == Job::Armorer
           || job == Job::CharBurner /* Comes from another file */)
        {
            const auto ware = randEnum<GoodType>();
            // Japanese shield is missing
            b.id = rttr::enum_cast((ware == GoodType::ShieldJapanese) ? GoodType::ShieldRomans : ware);
            b.hasWare = true;
        } else
        {
            // only native nations are loaded
            b.id = JOB_SPRITE_CONSTS[job].getBobId(Nation(rand() % NUM_NATIVE_NATIONS));
            b.hasWare = false;
        }

        b.pos.y = GetCtrl<ctrlButton>(0)->GetPos().y - 20 - rand() % 150;
        bobs.push_back(b);
    }

    // draw bobs
    for(const auto& bob : bobs)
    {
        if(!bob.hasWare)
            LOADER.GetBob("jobs")->Draw(bob.id, bob.direction, bob.isFat, bob.animationStep, bob.pos, bob.color);
        else
            LOADER.GetBob("carrier")->Draw(bob.id, bob.direction, bob.isFat, bob.animationStep, bob.pos, bob.color);
    }
}

void dskCredits::Msg_Timer(unsigned timerId)
{
    if(timerId == ID_tmrBobAnim)
    {
        const auto width = static_cast<int>(VIDEODRIVER.GetRenderSize().x);
        for(auto& bob : bobs)
        {
            bob.animationStep++;
            if(bob.animationStep > 7)
                bob.animationStep = 0;
            if(bob.direction == libsiedler2::ImgDir::E)
            {
                bob.pos.x += bob.speed;
                if(bob.pos.x > width)
                    bob.direction = libsiedler2::ImgDir::W;
            } else if(bob.direction == libsiedler2::ImgDir::W)
            {
                bob.pos.x -= bob.speed;
                if(bob.pos.x < 0)
                    bob.direction = libsiedler2::ImgDir::E;
            }
        }
    } else if(timerId == ID_tmrPage)
    {
        GotoNextPage();
    }
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

bool dskCredits::Msg_KeyDown(const KeyEvent&)
{
    GotoNextPage();
    return true;
}

bool dskCredits::Msg_LeftUp(const MouseCoords&)
{
    GotoNextPage();
    bobSpawnTimer.restart(); // Little easteregg
    return true;
}

bool dskCredits::Msg_RightUp(const MouseCoords&)
{
    GotoPrevPage();
    return true;
}

void dskCredits::Msg_ButtonClick(const unsigned)
{
    Close();
}

void dskCredits::Msg_MsgBoxResult(unsigned, MsgboxResult)
{
    Close();
}

void dskCredits::SetActive(bool active)
{
    Desktop::SetActive(active);
    if(active && !pageTimer->isRunning())
    {
        pageTimer->Start();
        bobSpawnTimer.start();
    }
}
