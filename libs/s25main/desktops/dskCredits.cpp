// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "dskCredits.h"
#include "LeatherLoader.h"
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

dskCredits::dskCredits() : Desktop(LOADER.getImageN("setup013", 0)), itCurEntry(entries.end())
{
    addTextButton(ID_btBack, DrawPoint(300, 550), Extent(200, 22), TextureColor::Red1, _("Back"), NormalFont);
    addText(ID_txtRttr, DrawPoint(400, 10), _("Return To The Roots"), COLOR_YELLOW, FontStyle::CENTER, LargeFont);
    addText(ID_txtCredits, DrawPoint(400, 33), _("Credits"), COLOR_YELLOW, FontStyle::CENTER, LargeFont);
    addTimer(ID_tmrBobAnim, 40ms);
    pageTimer = addTimer(ID_tmrPage, PAGE_TIME);
    pageTimer->stop();

    LOADER.Load(ResourceId("credits")); // Ignore failure

    // order by last name (alphabetical)
    CreditsEntry entry = CreditsEntry("Alexander Grund (Flamefire):", getCreditsImgOrDefault("flamefire"));
    entry.lines.push_back(_("Programming"));
    entry.lines.push_back(_("Quality Assurance"));
    entries.push_back(entry);

    entry = CreditsEntry("Patrick Haak (Demophobie):", getCreditsImgOrDefault("demophobie"));
    entry.lines.push_back(_("Website Administration"));
    entry.lines.push_back(_("Quality Assurance"));
    entries.push_back(entry);

    entry = CreditsEntry("Jan-Henrik Kluth (jh):", getCreditsImgOrDefault("jh"));
    entry.lines.push_back(_("Programming"));
    entry.lines.push_back(_("Artificial Intelligence (AI)"));
    entries.push_back(entry);

    entry = CreditsEntry("Christopher Kuehnel (Spikeone):", getCreditsImgOrDefault("spikeone"));
    entry.lines.push_back(_("Additional graphics"));
    entry.lines.push_back(_("Quality Assurance"));
    entry.lines.push_back(_("Mapping"));
    entries.push_back(entry);

    entry = CreditsEntry("Stefan Schüchl (Z-Stef):", getCreditsImgOrDefault("z-stef"));
    entry.lines.push_back(_("Website Administration"));
    entry.lines.push_back(_("Website Programming"));
    entries.push_back(entry);

    entry = CreditsEntry("Marcus Ströbel (Maqs):", getCreditsImgOrDefault("maqs"));
    entry.lines.push_back(_("Programming"));
    entry.lines.push_back(_("Quality Assurance"));
    entries.push_back(entry);

    entry = CreditsEntry("Jonas Trampe (NastX):", getCreditsImgOrDefault("nastx"));
    entry.lines.push_back(_("Quality Assurance"));
    entry.lines.push_back(_("Mapping"));
    entries.push_back(entry);

    // founder members
    entry = CreditsEntry("Oliver Siebert (Oliverr):", getCreditsImgOrDefault("oliverr"));
    entry.lines.push_back(_("Project founder"));
    entry.lines.push_back(_("Project management"));
    entry.lines.push_back(_("Programming"));
    entry.lines.push_back(_("Quality Assurance"));
    entries.push_back(entry);

    entry = CreditsEntry("Florian Doersch (FloSoft):", getCreditsImgOrDefault("flosoft"));
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
        WINDOWMANAGER.show(std::make_unique<iwMsgbox>(_("Error"), _("Failed to load game data"), this, MsgboxButton::Ok,
                                                      MsgboxIcon::ExclamationRed, 0));
        return;
    }

    std::vector<Nation> nations(NUM_NATIVE_NATIONS);
    for(unsigned i = 0; i < NUM_NATIVE_NATIONS; i++)
        nations[i] = Nation(i);

    if(!LOADER.loadFilesAtGame(worldDesc.get(DescIdx<LandscapeDesc>(0)).mapGfxPath, false, nations, {}))
    {
        WINDOWMANAGER.show(std::make_unique<iwMsgbox>(_("Error"), _("Failed to load game resources"), this,
                                                      MsgboxButton::Ok, MsgboxIcon::ExclamationRed, 0));
        return;
    }

    this->itCurEntry = entries.begin();

    if(LOADER.sng_lst.size() > 8)
        LOADER.sng_lst[8]->play(-1);
}

dskCredits::~dskCredits() = default;

void dskCredits::drawImpl()
{
    Desktop::drawImpl();
    if(itCurEntry != entries.end())
    {
        drawBobs();
        drawCredit();
    }
}

void dskCredits::gotoNextPage()
{
    ++itCurEntry;
    if(itCurEntry == entries.end())
        itCurEntry = entries.begin();
    pageTimer->start();
}

void dskCredits::gotoPrevPage()
{
    if(itCurEntry == entries.begin())
        itCurEntry = std::prev(entries.end());
    else
        --itCurEntry;
    pageTimer->start();
}

void dskCredits::drawCredit()
{
    const auto elapsedPageTime = std::chrono::duration_cast<std::chrono::milliseconds>(pageTimer->getElapsed());

    // calculate text transparency
    const auto fadeoutStartTime = PAGE_TIME - FADING_TIME;
    const unsigned transparency = (elapsedPageTime >= fadeoutStartTime) ?
                                    helpers::interpolate(0xFFu, 0u, elapsedPageTime - fadeoutStartTime, FADING_TIME) :
                                    helpers::interpolate(0u, 0xFFu, elapsedPageTime, FADING_TIME);

    // draw text
    LargeFont->draw(DrawPoint(40, 100), itCurEntry->title, FontStyle{}, SetAlpha(COLOR_RED, transparency));

    std::array<unsigned, 2> columnToY = {{150, 150}};

    for(const auto& line : itCurEntry->lines)
    {
        LargeFont->draw(DrawPoint(60 + line.column * 350, columnToY[line.column]), line.line, FontStyle{},
                        SetAlpha(COLOR_YELLOW, transparency));
        columnToY[line.column] += LargeFont->getHeight() + 5;
    }

    LargeFont->draw(DrawPoint(40, columnToY[0] + 20), itCurEntry->lastLine, FontStyle{},
                    SetAlpha(COLOR_RED, transparency));

    if(itCurEntry->pic)
        itCurEntry->pic->drawFull(DrawPoint(VIDEODRIVER.getRenderSize().x - 300, 70),
                                  SetAlpha(COLOR_WHITE, transparency));
}

template<typename T>
T randEnum()
{
    return T(rand() % helpers::NumEnumValues_v<T>);
}

void dskCredits::drawBobs()
{
    using namespace std::chrono_literals;
    auto bobSpawnInterval = 200ms;

    if(VIDEODRIVER.getFPS() < 30)
        bobSpawnInterval = 0ms;
    else if(VIDEODRIVER.getFPS() < 60)
        bobSpawnInterval = 1s;
    else if(VIDEODRIVER.getFPS() < 200)
        bobSpawnInterval = 500ms;

    const auto maxNumBos = 50 + VIDEODRIVER.getRenderSize().x / 2;

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
            b.pos.x = VIDEODRIVER.getRenderSize().x;
            b.direction = libsiedler2::ImgDir::W;
        }

        b.color = PLAYER_COLORS[rand() % PLAYER_COLORS.size()];

        auto isHeadlessBob = [](Job const& job) {
            return job == Job::Miller || job == Job::Baker || job == Job::Brewer || job == Job::Armorer;
        };

        auto isJobDrawable = [&isHeadlessBob](Job const& job) {
            if(job == Job::CharBurner)
                return false;
            if(wineaddon::isWineAddonJobType(job))
                return false;
            if(leatheraddon::isLeatherAddonJobType(job))
                return false;
            if(isHeadlessBob(job))
                return false;
            return true;
        };

        auto isWareDrawable = [](GoodType const& ware) {
            if(wineaddon::isWineAddonGoodType(ware))
                return false;
            if(leatheraddon::isLeatherAddonGoodType(ware))
                return false;
            return true;
        };

        const auto job = randEnum<Job>();
        if(isJobDrawable(job))
        {
            // only native nations are loaded
            b.id = JOB_SPRITE_CONSTS[job].getBobId(Nation(rand() % NUM_NATIVE_NATIONS));
            b.hasWare = false;
            b.pos.y = getCtrl<ctrlButton>(0)->getPos().y - 20 - rand() % 150;
            bobs.push_back(b);
        } else
        {
            const auto ware = randEnum<GoodType>();
            if(isWareDrawable(ware))
            {
                // Japanese shield is missing
                b.id = rttr::enum_cast((ware == GoodType::ShieldJapanese) ? GoodType::ShieldRomans : ware);
                b.hasWare = true;
                b.pos.y = getCtrl<ctrlButton>(0)->getPos().y - 20 - rand() % 150;
                bobs.push_back(b);
            }
        }
    }

    // draw bobs
    for(const auto& bob : bobs)
    {
        if(!bob.hasWare)
            LOADER.getBob("jobs")->draw(bob.id, bob.direction, bob.isFat, bob.animationStep, bob.pos, bob.color);
        else
            LOADER.getBob("carrier")->draw(bob.id, bob.direction, bob.isFat, bob.animationStep, bob.pos, bob.color);
    }
}

void dskCredits::msgTimer(unsigned timerId)
{
    if(timerId == ID_tmrBobAnim)
    {
        const auto width = static_cast<int>(VIDEODRIVER.getRenderSize().x);
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
        gotoNextPage();
    }
}

bool dskCredits::close()
{
    WINDOWMANAGER.Switch(std::make_unique<dskMainMenu>());
    return true;
}

glArchivItem_Bitmap* dskCredits::getCreditsImgOrDefault(const std::string& name)
{
    glArchivItem_Bitmap* result = LOADER.getImage("credits", name);
    if(!result)
        result = LOADER.getImage("credits", "default");
    return result;
}

bool dskCredits::msgKeyDown(const KeyEvent&)
{
    gotoNextPage();
    return true;
}

bool dskCredits::msgLeftUp(const MouseCoords&)
{
    gotoNextPage();
    bobSpawnTimer.restart(); // Little easteregg
    return true;
}

bool dskCredits::msgRightUp(const MouseCoords&)
{
    gotoPrevPage();
    return true;
}

void dskCredits::msgButtonClick(const unsigned)
{
    close();
}

void dskCredits::msgMsgBoxResult(unsigned, MsgboxResult)
{
    close();
}

void dskCredits::setActive(bool active)
{
    Desktop::setActive(active);
    if(active && !pageTimer->isRunning())
    {
        pageTimer->start();
        bobSpawnTimer.start();
    }
}
