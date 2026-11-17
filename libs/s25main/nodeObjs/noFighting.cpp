// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "noFighting.h"
#include "EventManager.h"
#include "GamePlayer.h"
#include "GlobalGameSettings.h"
#include "Loader.h"
#include "SerializedGameData.h"
#include "SoundManager.h"
#include "addons/const_addons.h"
#include "figures/nofActiveSoldier.h"
#include "network/GameClient.h"
#include "noSkeleton.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "ogl/glSmartBitmap.h"
#include "random/Random.h"
#include "world/GameWorld.h"
#include "gameData/MilitaryConsts.h"

noFighting::noFighting(nofActiveSoldier& soldier1, nofActiveSoldier& soldier2) : noBase(NodalObjectType::Fighting)
{
    RTTR_Assert(soldier1.getPlayer() != soldier2.getPlayer());
    const MapPoint pos = soldier1.getPos();

    soldiers[0] = world->removeFigure(pos, soldier1);
    soldiers[1] = world->removeFigure(pos, soldier2);
    turn = 2;
    defending_animation = 0;
    player_won = 0xFF;

    // Beginn-Event Anmelden (Soldaten gehen auf ihre Seiten)
    current_ev = getEvMgr().addEvent(this, 15);

    // anderen Leute, die auf diesem Punkt zulaufen, stoppen
    world->stopOnRoads(pos);

    // Sichtradius behalten
    world->makeVisibleAroundPoint(pos, VISUALRANGE_SOLDIER, soldier1.getPlayer());
    world->makeVisibleAroundPoint(pos, VISUALRANGE_SOLDIER, soldier2.getPlayer());
}

noFighting::~noFighting() = default;

void noFighting::serialize(SerializedGameData& sgd) const
{
    noBase::serialize(sgd);

    sgd.pushUnsignedChar(turn);
    sgd.pushUnsignedChar(defending_animation);
    sgd.pushEvent(current_ev);
    sgd.pushUnsignedChar(player_won);

    for(const auto& soldier : soldiers)
        sgd.pushObject(soldier);
}

noFighting::noFighting(SerializedGameData& sgd, const unsigned obj_id)
    : noBase(sgd, obj_id), turn(sgd.popUnsignedChar()), defending_animation(sgd.popUnsignedChar()),
      current_ev(sgd.popEvent()), player_won(sgd.popUnsignedChar())

{
    for(auto& soldier : soldiers)
        soldier.reset(sgd.popObject<nofActiveSoldier>());
}

void noFighting::destroy()
{
    RTTR_Assert(!soldiers[0]);
    RTTR_Assert(!soldiers[1]);
    noBase::destroy();
}

void noFighting::draw(DrawPoint drawPt)
{
    switch(turn)
    {
        case 3:
        case 4:
        {
            const unsigned curAnimFrame = GAMECLIENT.interpolate(16, current_ev);
            const auto& soldier = *soldiers[turn - 3];
            const GamePlayer& owner = world->getPlayer(soldier.getPlayer());

            // Sterben des einen Soldatens (letzte Phase)

            if(curAnimFrame < 4)
            {
                // Noch kurz dastehen und warten, bis man stirbt
                LOADER.fight_cache[owner.nation][soldier.getRank()][turn - 3].defending[0][0].drawForPlayer(
                  drawPt, owner.color);
            } else
            {
                // Sich in Luft auflösen
                if(turn == 3)
                    drawPt.x -= 12;
                else
                    drawPt.x += 12;
                LOADER.getPlayerImage("rom_bobs", 903 + curAnimFrame - 4)->drawFull(drawPt, COLOR_WHITE, owner.color);
            }
            if(soldiers[turn - 3]->hasArmor())
                soldiers[turn - 3]->drawArmor(drawPt);

            // Sterbesound abspielen
            if(curAnimFrame == 6)
                world->getSoundMgr().playNOSound(104, *this, 2);
        }
        break;
        case 2:
        {
            // Erste Phase des Kampfes, die Soldaten gehen jeweils nach links bzw. rechts
            auto xDiff = int(GAMECLIENT.interpolate(12, current_ev));
            drawPt.x -= xDiff;
            for(unsigned i : {0, 1})
            {
                const GamePlayer& owner = world->getPlayer(soldiers[i]->getPlayer());
                glSmartBitmap& bmp = LOADER.getBobSprite(owner.nation, soldiers[i]->getJobType(),
                                                         (i == 0) ? Direction::West : Direction::East,
                                                         GAMECLIENT.interpolate(8, current_ev));
                bmp.draw(drawPt, COLOR_WHITE, owner.color);
                if(soldiers[i]->hasArmor())
                    soldiers[i]->drawArmor(drawPt);
                drawPt.x += 2 * xDiff;
            }
        }
        break;
        default:
        {
            // Kampf
            // Aktueller Animationsframe
            const unsigned curAnimFrame = GAMECLIENT.interpolate(8, current_ev);

            for(unsigned i : {0, 1})
            {
                const auto& soldier = *soldiers[i];
                const GamePlayer& owner = world->getPlayer(soldier.getPlayer());
                auto& fightAnim = LOADER.fight_cache[owner.nation][soldier.getRank()][i];

                // Ist der Soldat mit Angreifen dran?
                if(turn == i)
                {
                    // Angreifen
                    fightAnim.attacking[curAnimFrame].drawForPlayer(drawPt, owner.color);
                } else
                {
                    // Verteidigen
                    if(defending_animation < 3)
                    {
                        // Verteidigungsanimation
                        fightAnim.defending[defending_animation][curAnimFrame].drawForPlayer(drawPt, owner.color);

                        // Wenn schwache Soldaten Schild hinhalten (Ani 0 und 1) oder stärkere sich mit den Schwertern
                        // schützen (Ani 0) dann Schwert-aneinanderklirr-Sound abspielen
                        if(curAnimFrame == 5
                           && ((soldier.getRank() < 2 && defending_animation < 2)
                               || (soldier.getRank() > 1 && defending_animation == 0)))
                            world->getSoundMgr().playNOSound(101, *this, 1);

                    } else
                    {
                        // Getroffen-Animation (weißes Aufblinken)
                        if(curAnimFrame == HIT_MOMENT[soldiers[1 - i]->getRank()])
                        {
                            // weiß aufblinken
                            fightAnim.hit.drawForPlayer(drawPt, owner.color);

                            // Treffersound
                            world->getSoundMgr().playNOSound(105, *this, 1);
                        } else
                            // normal dastehen
                            fightAnim.defending[0][0].drawForPlayer(drawPt, owner.color);
                    }
                }
                if(soldiers[i]->hasArmor())
                    soldiers[i]->drawArmor(drawPt + DrawPoint(i == 0 ? -12 : 12, 0));
            }

            // Angriffssound
            if(curAnimFrame == 3)
                world->getSoundMgr().playNOSound(103, *this, 0);
        }
        break;
    }
}

void noFighting::handleEvent(const unsigned id)
{
    // Normales Ablaufevent?
    if(id == 0)
    {
        switch(turn)
        {
            case 2:
            {
                // Der Kampf hat gerade begonnen

                // "Auslosen", wer als erstes dran ist mit Angreifen
                turn = static_cast<unsigned char>(RANDOM_RAND(2));
                // anfangen anzugreifen
                startAttack();
            }
                return;
            case 0:
            case 1:
            {
                // Sounds löschen von der letzten Kampfphase
                world->getSoundMgr().stopSounds(*this);

                // Wurde der eine getroffen?
                if(defending_animation == 3)
                {
                    soldiers[1 - turn]->takeHit();
                    if(soldiers[1 - turn]->getHitpoints() == 0)
                    {
                        // Besitzer merken für die Sichtbarkeiten am Ende dann
                        player_won = soldiers[turn]->getPlayer();
                        // Soldat Bescheid sagen, dass er stirbt
                        soldiers[1 - turn]->lostFighting();
                        // Anderen Soldaten auf die Karte wieder setzen, Bescheid sagen, er kann wieder loslaufen
                        const MapPoint pos = soldiers[turn]->getPos();
                        auto& winningSoldier = world->addFigure(pos, std::move(soldiers[turn]));
                        // Hitpoints sind 0 --> Soldat ist tot, Kampf beendet, turn = 3+welche Soldat stirbt
                        // Do this before calling wonFighting so this fight doesn't block the soldier looking for a new
                        // fight spot
                        turn = 3 + (1 - turn);
                        winningSoldier.wonFighting();
                        // Event zum Sterben des einen Soldaten anmelden
                        current_ev = getEvMgr().addEvent(this, 30);
                        // Umstehenden Figuren Bescheid Bescheid sagen
                        world->roadNodeAvailable(pos);

                        // In die Statistik eintragen
                        world->getPlayer(player_won).changeStatisticValue(StatisticType::Vanquished, 1);
                        return;
                    }
                }

                turn = 1 - turn;
                startAttack();
            }
                return;
            case 3:
            case 4:
            {
                unsigned playerLost = turn - 3;
                MapPoint pt = soldiers[playerLost]->getPos();

                // Sounds löschen vom Sterben
                world->getSoundMgr().stopSounds(*this);

                // Kampf ist endgültig beendet
                getEvMgr().addToKillList(world->removeFigure(pt, *this));

                // Wenn da nix war bzw. nur ein Verzierungsobjekt, kommt nun ein Skelett hin
                NodalObjectType noType = world->getNO(pt)->getType();
                if(noType == NodalObjectType::Nothing || noType == NodalObjectType::Environment)
                {
                    world->destroyNO(pt, false);
                    world->setNO(pt, new noSkeleton(pt));
                }

                // Sichtradius ausblenden am Ende des Kampfes, an jeweiligen Soldaten dann übergeben, welcher überlebt
                // hat
                world->recalcVisibilitiesAroundPoint(pt, VISUALRANGE_SOLDIER, soldiers[playerLost]->getPlayer(),
                                                     nullptr);
                world->recalcVisibilitiesAroundPoint(pt, VISUALRANGE_SOLDIER, player_won, nullptr);

                // Soldaten endgültig umbringen
                world->getPlayer(soldiers[playerLost]->getPlayer())
                  .decreaseInventoryJob(soldiers[playerLost]->getJobType(), 1);
                soldiers[playerLost]->destroy();
                soldiers[playerLost].reset();
            }
            break;
        }
    } else
        RTTR_Assert(false);
}

void noFighting::startAttack()
{
    // "Auswürfeln", ob der Angreifer (also der, der gerade den Angriff vollzieht) trifft oder ob sich der andere
    // erfolgreich verteidigt

    std::array<unsigned char, 2> results;
    for(unsigned i = 0; i < 2; ++i)
    {
        switch(world->getGGS().getSelection(AddonId::ADJUST_MILITARY_STRENGTH))
        {
            case 0: // Maximale Stärke
                results[i] = RANDOM_RAND(soldiers[i]->getRank() + 6);
                break;
            case 1: // Mittlere Stärke
            default: results[i] = RANDOM_RAND(soldiers[i]->getRank() + 10); break;
            case 2: // Minimale Stärke
                results[i] = RANDOM_RAND(10);
                break;
        }
    }

    if((turn == 0 && results[0] > results[1]) || (turn == 1 && results[1] > results[0]))
        // Der Angreifer hat diesen Zug gewonnen
        defending_animation = 3;
    else
        // Der Verteidiger hat diesen Zug gewonnen, zufällige Verteidigungsanimation
        defending_animation = static_cast<unsigned char>(RANDOM_RAND(3));

    // Entsprechendes Event anmelden
    current_ev = getEvMgr().addEvent(this, 15);
}

bool noFighting::isActive() const
{
    return turn < 3;
}

bool noFighting::isSoldierOfPlayer(const unsigned char player) const
{
    for(const std::unique_ptr<nofActiveSoldier>& soldier : soldiers)
    {
        if(soldier && soldier->getPlayer() == player)
            return true;
    }

    // Der Spieler der gewonnen hat und schon wieder gegangen ist (taucht dann nicht bei den ersten beiden mit
    // auf, wenn der Kampf beendet ist)
    return player_won == player;
}
