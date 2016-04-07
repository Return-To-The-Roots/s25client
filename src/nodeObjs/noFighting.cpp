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
#include "noFighting.h"
#include "gameData/MilitaryConsts.h"

#include "figures/nofActiveSoldier.h"
#include "Random.h"
#include "EventManager.h"
#include "GameClient.h"
#include "Loader.h"
#include "noSkeleton.h"
#include "SoundManager.h"
#include "SerializedGameData.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "ogl/glSmartBitmap.h"

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

noFighting::noFighting(nofActiveSoldier* soldier1, nofActiveSoldier* soldier2) : noBase(NOP_FIGHTING)
{
    RTTR_Assert(soldier1->GetPlayer() != soldier2->GetPlayer());

    soldiers[0] = soldier1;
    soldiers[1] = soldier2;
    turn = 2;
    defending_animation = 0;
    player_won = 0xFF;

    // Die beiden Soldaten erstmal aus der Liste hauen
    gwg->RemoveFigure(soldier1, soldier1->GetPos());
    gwg->RemoveFigure(soldier2, soldier1->GetPos());

    // Beginn-Event Anmelden (Soldaten gehen auf ihre Seiten)
    current_ev = em->AddEvent(this, 15);

    // anderen Leute, die auf diesem Punkt zulaufen, stoppen
    gwg->StopOnRoads(soldier1->GetPos());

    // Sichtradius behalten
    gwg->SetVisibilitiesAroundPoint(soldier1->GetPos(), VISUALRANGE_SOLDIER, soldier1->GetPlayer());
    gwg->SetVisibilitiesAroundPoint(soldier1->GetPos(), VISUALRANGE_SOLDIER, soldier2->GetPlayer());
}

noFighting::~noFighting()
{
    deletePtr(soldiers[0]);
    deletePtr(soldiers[1]);
}

void noFighting::Serialize_noFighting(SerializedGameData& sgd) const
{
    Serialize_noBase(sgd);

    sgd.PushUnsignedChar(turn);
    sgd.PushUnsignedChar(defending_animation);
    sgd.PushObject(current_ev, true);
    sgd.PushUnsignedChar(player_won);

    for(unsigned i = 0; i < 2; ++i)
        sgd.PushObject(soldiers[i], false);
}

noFighting::noFighting(SerializedGameData& sgd, const unsigned obj_id) : noBase(sgd, obj_id),
    turn(sgd.PopUnsignedChar()),
    defending_animation(sgd.PopUnsignedChar()),
    current_ev(sgd.PopObject<EventManager::Event>(GOT_EVENT)),
    player_won(sgd.PopUnsignedChar())

{
    for(unsigned i = 0; i < 2; ++i)
        soldiers[i] = sgd.PopObject<nofActiveSoldier>(GOT_UNKNOWN);
}

void noFighting::Destroy_noFighting()
{
    RTTR_Assert(!soldiers[0]);
    RTTR_Assert(!soldiers[1]);
    Destroy_noBase();
}

void noFighting::Draw(int x, int y)
{
    switch(turn)
    {
        case 3:
        case 4:
        {
            unsigned animation = GAMECLIENT.Interpolate(16, current_ev);

            // Sterben des einen Soldatens (letzte Phase)

            if(animation < 4)
            {
                // Noch kurz dastehen und warten, bis man stirbt
                glArchivItem_Bitmap_Player* image = LOADER.GetPlayerImage("rom_bobs", FIGHT_ANIMATIONS[gwg->GetPlayer(soldiers[turn - 3]->GetPlayer()).nation][soldiers[turn - 3]->GetRank()][turn - 3].defending[0][0]);
                image->Draw(x, y, 0, 0, 0, 0, 0, 0, COLOR_WHITE, gwg->GetPlayer(soldiers[turn - 3]->GetPlayer()).color);
            }
            else
                // Sich in Luft auflösen
                LOADER.GetPlayerImage("rom_bobs", 903 + animation - 4)->Draw(x + ((turn - 3 == 0) ? (-12) : 12), y, 0, 0, 0, 0, 0, 0, COLOR_WHITE, gwg->GetPlayer(soldiers[turn - 3]->GetPlayer()).color);

            // Sterbesound abspielen
            if(animation == 6)
                SOUNDMANAGER.PlayNOSound(104, this, 2);


        } break;
        case 2:
        {
            // Erste Phase des Kampfes, die Soldaten gehen jeweils nach links bzw. rechts
            int x_diff = int(GAMECLIENT.Interpolate(12, current_ev));

            for(unsigned i = 0; i < 2; ++i)
            {
                GameClientPlayer& owner = gwg->GetPlayer(soldiers[i]->GetPlayer());
                glSmartBitmap& bmp = LOADER.bob_jobs_cache[owner.nation][soldiers[i]->GetJobType()][(i == 0) ? 0 : 3][GAMECLIENT.Interpolate(8, current_ev)];
                bmp.draw(x + ((i == 0) ? (-x_diff) : x_diff), y, COLOR_WHITE, owner.color);
            }

        } break;
        default:
        {
            // Kampf
            // Aktueller Animationsframe
            unsigned animation = GAMECLIENT.Interpolate(8, current_ev);

            for(unsigned i = 0; i < 2; ++i)
            {
                // Ist der Soldat mit Angreifen dran?
                if(turn == i)
                {
                    // Angreifen
                    LOADER.GetPlayerImage("rom_bobs",
                                     FIGHT_ANIMATIONS[gwg->GetPlayer(soldiers[i]->GetPlayer()).nation][soldiers[i]->GetRank()][i].
                                     attacking[animation])->Draw(x, y, 0, 0, 0, 0, 0, 0, COLOR_WHITE, gwg->GetPlayer(soldiers[i]->GetPlayer()).color);
                }
                else
                {
                    // Verteidigen
                    if(defending_animation < 3)
                    {
                        // Verteidigungsanimation
                        LOADER.GetPlayerImage("rom_bobs",
                                         FIGHT_ANIMATIONS[gwg->GetPlayer(soldiers[i]->GetPlayer()).nation][soldiers[i]->GetRank()][i].
                                         defending[defending_animation][animation])->Draw(x, y, 0, 0, 0, 0, 0, 0, COLOR_WHITE, gwg->GetPlayer(soldiers[i]->GetPlayer()).color);

                        // Wenn schwache Soldaten Schild hinhalten (Ani 0 und 1) und stärkere sich mit den Schwertern schützen (Ani 0)
                        // dann Schwert-aneinanderklirr-Sound abspielen
                        if( (animation == 5) && ((soldiers[i]->GetRank() < 2 && (defending_animation < 2)) || (soldiers[i]->GetRank() > 1 && (defending_animation == 0))))
                            SOUNDMANAGER.PlayNOSound(101, this, 1);

                    }
                    else
                    {
                        // Getroffen-Animation (weißes Aufblinken)
                        if(GAMECLIENT.Interpolate(8, current_ev) == HIT_MOMENT[soldiers[!i]->GetRank()])
                        {
                            // weiß aufblinken
                            LOADER.GetPlayerImage("rom_bobs",
                                             HIT_SOLDIERS[gwg->GetPlayer(soldiers[i]->GetPlayer()).nation][soldiers[i]->GetRank()] + i)
                            ->Draw(x, y, 0, 0, 0, 0, 0, 0, COLOR_WHITE, gwg->GetPlayer(soldiers[i]->GetPlayer()).color);

                            // Treffersound
                            SOUNDMANAGER.PlayNOSound(105, this, 1);
                        }
                        else
                            // normal dastehen
                            LOADER.GetPlayerImage("rom_bobs",
                                             FIGHT_ANIMATIONS[gwg->GetPlayer(soldiers[i]->GetPlayer()).nation][soldiers[i]->GetRank()][i].
                                             defending[0][0])->Draw(x, y, 0, 0, 0, 0, 0, 0, COLOR_WHITE, gwg->GetPlayer(soldiers[i]->GetPlayer()).color);
                    }
                }
            }

            // Angriffssound
            if(animation == 3)
                SOUNDMANAGER.PlayNOSound(103, this, 0);



        } break;
    }

}

void noFighting::HandleEvent(const unsigned int id)
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
                turn = static_cast<unsigned char>(RANDOM.Rand(__FILE__, __LINE__, GetObjId(), 2));
                // anfangen anzugreifen
                StartAttack();
            } return;
            case 0:
            case 1:
            {
                // Sounds löschen von der letzten Kampfphase
                SOUNDMANAGER.WorkingFinished(this);

                // Wurde der eine getroffen?
                if(defending_animation == 3)
                {
                    if(--soldiers[!turn]->hitpoints == 0)
                    {
                        // Besitzer merken für die Sichtbarkeiten am Ende dann
                        player_won = soldiers[turn]->GetPlayer();
                        // Soldat Bescheid sagen, dass er stirbt
                        soldiers[!turn]->LostFighting();
                        // Anderen Soldaten auf die Karte wieder setzen, Bescheid sagen, er kann wieder loslaufen
                        gwg->AddFigure(soldiers[turn], soldiers[turn]->GetPos());
                        soldiers[turn]->WonFighting();
                        soldiers[turn] = NULL;
                        // Hitpoints sind 0 --> Soldat ist tot, Kampf beendet, turn = 3+welche Soldat stirbt
                        turn = 3 + (!turn);
                        // Event zum Sterben des einen Soldaten anmelden
                        current_ev = em->AddEvent(this, 30);
                        // Umstehenden Figuren Bescheid nach gewisser Zeit Bescheid sagen
                        /*em->AddEvent(this,RELEASE_FIGURES_OFFSET,1);*/
                        gwg->RoadNodeAvailable(soldiers[turn - 3]->GetPos());

                        // In die Statistik eintragen
                        gwg->GetPlayer(player_won).ChangeStatisticValue(STAT_VANQUISHED, 1);
                        return;
                    }
                }

                turn = !turn;
                StartAttack();
            } return;
            case 3:
            case 4:
            {
                unsigned player_lost = turn - 3;
                MapPoint pt = soldiers[player_lost]->GetPos();

                // Sounds löschen vom Sterben
                SOUNDMANAGER.WorkingFinished(this);

                // Kampf ist endgültig beendet
                em->AddToKillList(this);
                gwg->RemoveFigure(this, pt);

                // Wenn da nix war bzw. nur ein Verzierungsobjekt, kommt nun ein Skelett hin
                NodalObjectType noType = gwg->GetNO(pt)->GetType();
                if(noType == NOP_NOTHING || noType == NOP_ENVIRONMENT)
                {
                    gwg->DestroyNO(pt, false);
                    gwg->SetNO(pt, new noSkeleton(pt));
                }

                // Umstehenden Figuren Bescheid sagen
                //gwg->RoadNodeAvailable(soldiers[turn-3]->GetX(),soldiers[turn-3]->GetY());

                // Sichtradius ausblenden am Ende des Kampfes, an jeweiligen Soldaten dann übergeben, welcher überlebt hat
                gwg->RecalcVisibilitiesAroundPoint(pt, VISUALRANGE_SOLDIER, soldiers[player_lost]->GetPlayer(), NULL);
                gwg->RecalcVisibilitiesAroundPoint(pt, VISUALRANGE_SOLDIER, player_won, NULL);

                // Soldaten endgültig umbringen
                gwg->GetPlayer(soldiers[player_lost]->GetPlayer()).DecreaseInventoryJob(soldiers[player_lost]->GetJobType(), 1);
                soldiers[player_lost]->Destroy();
                deletePtr(soldiers[player_lost]);

            } break;
        }
    }
    // Figur-Weiterlauf-Event
    else if(id == 1)
    {
        //// Figuren weiterlaufen lassen
        //gwg->RoadNodeAvailable(soldiers[turn-3]->GetX(),soldiers[turn-3]->GetY());
    }
}

void noFighting::StartAttack()
{
    // "Auswürfeln", ob der Angreifer (also der, der gerade den Angriff vollzieht) trifft oder ob sich der andere
    // erfolgreich verteidigt

    unsigned char results[2];
    for(unsigned i = 0; i < 2; ++i)
    {
        switch (GAMECLIENT.GetGGS().getSelection(AddonId::ADJUST_MILITARY_STRENGTH))
        {
            case 0: // Maximale Stärke
            {
                results[i] = RANDOM.Rand(__FILE__, __LINE__, GetObjId(), soldiers[i]->GetRank() + 6);
            } break;
            case 1: // Mittlere Stärke
            default:
            {
                results[i] = RANDOM.Rand(__FILE__, __LINE__, GetObjId(), soldiers[i]->GetRank() + 10);
            } break;
            case 2: // Minimale Stärke
            {
                results[i] = RANDOM.Rand(__FILE__, __LINE__, GetObjId(), 10);
            } break;
        }
    }

    if((turn == 0 && results[0] > results[1])
            || (turn == 1 && results[1] > results[0]))
        // Der Angreifer hat diesen Zug gewonnen
        defending_animation = 3;
    else
        // Der Verteidiger hat diesen Zug gewonnen, zufällige Verteidigungsanimation
        defending_animation = static_cast<unsigned char>(RANDOM.Rand(__FILE__, __LINE__, GetObjId(), 3));

    // Entsprechendes Event anmelden
    current_ev = em->AddEvent(this, 15);

}

bool noFighting::IsActive() const
{
    // Figuren dürfen vorbei, wenn Kampf an sich und die Offset-Zeit abgelaufen ist
    return (turn < 3/* || GAMECLIENT.GetGFNumber()-current_ev->gf < RELEASE_FIGURES_OFFSET*/);
}

bool noFighting::IsSoldierOfPlayer(const unsigned char player) const
{
    // Soldat 1 prüfen
    if(soldiers[0])
    {
        if(soldiers[0]->GetPlayer() == player)
            return true;
    }

    // Soldat 2 prüfen
    if(soldiers[1])
    {
        if(soldiers[1]->GetPlayer() == player)
            return true;
    }

    // Der Spieler der gewonnen hat und schon wieder gegangen ist (taucht dann nicht bei den ersten beiden mit
    // auf, wenn der Kampf beendet ist)
    return player_won == player;
}

