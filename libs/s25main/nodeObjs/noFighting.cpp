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

noFighting::noFighting(nofActiveSoldier* soldier1, nofActiveSoldier* soldier2) : noBase(NodalObjectType::Fighting)
{
    RTTR_Assert(soldier1->GetPlayer() != soldier2->GetPlayer());

    soldiers[0] = soldier1;
    soldiers[1] = soldier2;
    turn = 2;
    defending_animation = 0;
    player_won = 0xFF;

    // Die beiden Soldaten erstmal aus der Liste hauen
    world->RemoveFigure(soldier1->GetPos(), soldier1);
    world->RemoveFigure(soldier1->GetPos(), soldier2);

    // Beginn-Event Anmelden (Soldaten gehen auf ihre Seiten)
    current_ev = GetEvMgr().AddEvent(this, 15);

    // anderen Leute, die auf diesem Punkt zulaufen, stoppen
    world->StopOnRoads(soldier1->GetPos());

    // Sichtradius behalten
    world->MakeVisibleAroundPoint(soldier1->GetPos(), VISUALRANGE_SOLDIER, soldier1->GetPlayer());
    world->MakeVisibleAroundPoint(soldier1->GetPos(), VISUALRANGE_SOLDIER, soldier2->GetPlayer());
}

noFighting::~noFighting()
{
    deletePtr(soldiers[0]);
    deletePtr(soldiers[1]);
}

void noFighting::Serialize(SerializedGameData& sgd) const
{
    noBase::Serialize(sgd);

    sgd.PushUnsignedChar(turn);
    sgd.PushUnsignedChar(defending_animation);
    sgd.PushEvent(current_ev);
    sgd.PushUnsignedChar(player_won);

    for(auto* soldier : soldiers)
        sgd.PushObject(soldier);
}

noFighting::noFighting(SerializedGameData& sgd, const unsigned obj_id)
    : noBase(sgd, obj_id), turn(sgd.PopUnsignedChar()), defending_animation(sgd.PopUnsignedChar()),
      current_ev(sgd.PopEvent()), player_won(sgd.PopUnsignedChar())

{
    for(auto& soldier : soldiers)
        soldier = sgd.PopObject<nofActiveSoldier>();
}

void noFighting::Destroy()
{
    RTTR_Assert(!soldiers[0]);
    RTTR_Assert(!soldiers[1]);
    noBase::Destroy();
}

void noFighting::Draw(DrawPoint drawPt)
{
    switch(turn)
    {
        case 3:
        case 4:
        {
            const unsigned curAnimFrame = GAMECLIENT.Interpolate(16, current_ev);
            const auto& soldier = *soldiers[turn - 3];
            const GamePlayer& owner = world->GetPlayer(soldier.GetPlayer());

            // Sterben des einen Soldatens (letzte Phase)

            if(curAnimFrame < 4)
            {
                // Noch kurz dastehen und warten, bis man stirbt
                LOADER.fight_cache[owner.nation][soldier.GetRank()][turn - 3].defending[0][0].drawForPlayer(
                  drawPt, owner.color);
            } else
            {
                // Sich in Luft auflösen
                if(turn == 3)
                    drawPt.x -= 12;
                else
                    drawPt.x += 12;
                LOADER.GetPlayerImage("rom_bobs", 903 + curAnimFrame - 4)->DrawFull(drawPt, COLOR_WHITE, owner.color);
            }

            // Sterbesound abspielen
            if(curAnimFrame == 6)
                gwg->GetSoundMgr().playNOSound(104, *this, 2);
        }
        break;
        case 2:
        {
            // Erste Phase des Kampfes, die Soldaten gehen jeweils nach links bzw. rechts
            auto x_diff = int(GAMECLIENT.Interpolate(12, current_ev));
            drawPt.x -= x_diff;
            for(unsigned i : {0, 1})
            {
                const GamePlayer& owner = world->GetPlayer(soldiers[i]->GetPlayer());
                glSmartBitmap& bmp = LOADER.getBobSprite(owner.nation, soldiers[i]->GetJobType(),
                                                         (i == 0) ? Direction::West : Direction::East,
                                                         GAMECLIENT.Interpolate(8, current_ev));
                bmp.draw(drawPt, COLOR_WHITE, owner.color);
                drawPt.x += 2 * x_diff;
            }
        }
        break;
        default:
        {
            // Kampf
            // Aktueller Animationsframe
            const unsigned curAnimFrame = GAMECLIENT.Interpolate(8, current_ev);

            for(unsigned i : {0, 1})
            {
                const auto& soldier = *soldiers[i];
                const GamePlayer& owner = world->GetPlayer(soldier.GetPlayer());
                auto& fightAnim = LOADER.fight_cache[owner.nation][soldier.GetRank()][i];

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
                           && ((soldier.GetRank() < 2 && defending_animation < 2)
                               || (soldier.GetRank() > 1 && defending_animation == 0)))
                            gwg->GetSoundMgr().playNOSound(101, *this, 1);

                    } else
                    {
                        // Getroffen-Animation (weißes Aufblinken)
                        if(curAnimFrame == HIT_MOMENT[soldiers[1 - i]->GetRank()])
                        {
                            // weiß aufblinken
                            fightAnim.hit.drawForPlayer(drawPt, owner.color);

                            // Treffersound
                            gwg->GetSoundMgr().playNOSound(105, *this, 1);
                        } else
                            // normal dastehen
                            fightAnim.defending[0][0].drawForPlayer(drawPt, owner.color);
                    }
                }
            }

            // Angriffssound
            if(curAnimFrame == 3)
                gwg->GetSoundMgr().playNOSound(103, *this, 0);
        }
        break;
    }
}

void noFighting::HandleEvent(const unsigned id)
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
                StartAttack();
            }
                return;
            case 0:
            case 1:
            {
                // Sounds löschen von der letzten Kampfphase
                gwg->GetSoundMgr().stopSounds(*this);

                // Wurde der eine getroffen?
                if(defending_animation == 3)
                {
                    soldiers[1 - turn]->TakeHit();
                    if(soldiers[1 - turn]->GetHitpoints() == 0)
                    {
                        // Besitzer merken für die Sichtbarkeiten am Ende dann
                        player_won = soldiers[turn]->GetPlayer();
                        // Soldat Bescheid sagen, dass er stirbt
                        soldiers[1 - turn]->LostFighting();
                        // Anderen Soldaten auf die Karte wieder setzen, Bescheid sagen, er kann wieder loslaufen
                        world->AddFigure(soldiers[turn]->GetPos(), soldiers[turn]);
                        soldiers[turn]->WonFighting();
                        soldiers[turn] = nullptr;
                        // Hitpoints sind 0 --> Soldat ist tot, Kampf beendet, turn = 3+welche Soldat stirbt
                        turn = 3 + (1 - turn);
                        // Event zum Sterben des einen Soldaten anmelden
                        current_ev = GetEvMgr().AddEvent(this, 30);
                        // Umstehenden Figuren Bescheid Bescheid sagen
                        world->RoadNodeAvailable(soldiers[turn - 3]->GetPos());

                        // In die Statistik eintragen
                        world->GetPlayer(player_won).ChangeStatisticValue(StatisticType::Vanquished, 1);
                        return;
                    }
                }

                turn = 1 - turn;
                StartAttack();
            }
                return;
            case 3:
            case 4:
            {
                unsigned player_lost = turn - 3;
                MapPoint pt = soldiers[player_lost]->GetPos();

                // Sounds löschen vom Sterben
                gwg->GetSoundMgr().stopSounds(*this);

                // Kampf ist endgültig beendet
                GetEvMgr().AddToKillList(this);
                world->RemoveFigure(pt, this);

                // Wenn da nix war bzw. nur ein Verzierungsobjekt, kommt nun ein Skelett hin
                NodalObjectType noType = world->GetNO(pt)->GetType();
                if(noType == NodalObjectType::Nothing || noType == NodalObjectType::Environment)
                {
                    world->DestroyNO(pt, false);
                    world->SetNO(pt, new noSkeleton(pt));
                }

                // Sichtradius ausblenden am Ende des Kampfes, an jeweiligen Soldaten dann übergeben, welcher überlebt
                // hat
                world->RecalcVisibilitiesAroundPoint(pt, VISUALRANGE_SOLDIER, soldiers[player_lost]->GetPlayer(),
                                                     nullptr);
                world->RecalcVisibilitiesAroundPoint(pt, VISUALRANGE_SOLDIER, player_won, nullptr);

                // Soldaten endgültig umbringen
                world->GetPlayer(soldiers[player_lost]->GetPlayer())
                  .DecreaseInventoryJob(soldiers[player_lost]->GetJobType(), 1);
                soldiers[player_lost]->Destroy();
                deletePtr(soldiers[player_lost]);
            }
            break;
        }
    } else
        RTTR_Assert(false);
}

void noFighting::StartAttack()
{
    // "Auswürfeln", ob der Angreifer (also der, der gerade den Angriff vollzieht) trifft oder ob sich der andere
    // erfolgreich verteidigt

    std::array<unsigned char, 2> results;
    for(unsigned i = 0; i < 2; ++i)
    {
        switch(world->GetGGS().getSelection(AddonId::ADJUST_MILITARY_STRENGTH))
        {
            case 0: // Maximale Stärke
                results[i] = RANDOM_RAND(soldiers[i]->GetRank() + 6);
                break;
            case 1: // Mittlere Stärke
            default: results[i] = RANDOM_RAND(soldiers[i]->GetRank() + 10); break;
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
    current_ev = GetEvMgr().AddEvent(this, 15);
}

bool noFighting::IsActive() const
{
    return turn < 3;
}

bool noFighting::IsSoldierOfPlayer(const unsigned char player) const
{
    for(const nofSoldier* soldier : soldiers)
    {
        if(soldier && soldier->GetPlayer() == player)
            return true;
    }

    // Der Spieler der gewonnen hat und schon wieder gegangen ist (taucht dann nicht bei den ersten beiden mit
    // auf, wenn der Kampf beendet ist)
    return player_won == player;
}
