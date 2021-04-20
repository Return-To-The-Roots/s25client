// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "DrawPoint.h"
#include "helpers/MultiArray.h"
#include "gameTypes/JobTypes.h"
#include "gameData/NationConsts.h"
#include <array>

/// Größe der Militärquadrate (in Knotenpunkten), in die die Welt eingeteilt wurde für Militärgebäude
constexpr uint16_t MILITARY_SQUARE_SIZE = 20;

/// Maximale Entfernungen für "nahe Militärgebäudedistanz" und "mittlere Militärgebäudedistanz"
constexpr unsigned MAX_MILITARY_DISTANCE_NEAR = 18;
constexpr unsigned MAX_MILITARY_DISTANCE_MIDDLE = 26;

/// highest military rank - currently ranks 0-4 available
constexpr unsigned MAX_MILITARY_RANK = NUM_SOLDIER_RANKS - 1u;
/// Number of military buildings
constexpr unsigned NUM_MILITARY_BLDS = 4;

/// Basisangriffsreichweite (Angriff mit allen Soldaten möglich)
constexpr unsigned BASE_ATTACKING_DISTANCE = 21;

/// Erweiterte Reichweite, für die jeweils ein Soldat von der Angriffsarmee abgezogen wird
constexpr unsigned EXTENDED_ATTACKING_DISTANCE = 1;

/// Maximale Länge für den Laufweg beim Angriff
constexpr unsigned MAX_ATTACKING_RUN_DISTANCE = 40;

/// Distanz zwischen zwei Gegnern, sodass diese aufeinander zugehen
constexpr unsigned MEET_FOR_FIGHT_DISTANCE = 5;

/// Besatzung in den einzelnen Militärgebäuden und nach Nation
constexpr helpers::EnumArray<std::array<int, NUM_MILITARY_BLDS>, Nation> NUM_TROOPS = {
  {{2, 3, 6, 9}, {2, 3, 6, 9}, {2, 3, 6, 9}, {2, 3, 6, 9}, {2, 3, 6, 9}}};

/// Gold in den einzelnen Militärgebäuden und nach Nation
constexpr helpers::EnumArray<std::array<int, NUM_MILITARY_BLDS>, Nation> NUM_GOLDS = {
  {{1, 2, 4, 6}, {1, 2, 4, 6}, {1, 2, 4, 6}, {1, 2, 4, 6}, {1, 2, 4, 6}}};

/// Radien der Militärgebäude
constexpr std::array<unsigned, NUM_MILITARY_BLDS> SUPPRESS_UNUSED MILITARY_RADIUS = {{8, 9, 10, 11}};
// Radius für einzelne Hafen(baustellen)
constexpr unsigned HARBOR_RADIUS = 8;
constexpr unsigned HQ_RADIUS = 9;

/// Offset of the troop flag per nation and type from the buildings origin
constexpr helpers::EnumArray<std::array<DrawPoint, NUM_MILITARY_BLDS>, Nation> TROOPS_FLAG_OFFSET = {
  {{{{24, -41}, {19, -41}, {31, -88}, {35, -67}}},
   {{{-9, -49}, {14, -59}, {16, -63}, {0, -44}}},
   {{{-24, -36}, {9, -62}, {-2, -80}, {23, -75}}},
   {{{-5, -50}, {-5, -51}, {-9, -74}, {-12, -58}}},
   {{{-22, -37}, {-2, -51}, {20, -70}, {-46, -64}}}}};

/// Offset of the troop flag per nation from the HQs origin
constexpr helpers::EnumArray<DrawPoint, Nation> TROOPS_FLAG_HQ_OFFSET = {
  {{-12, -102}, {-19, -94}, {-18, -112}, {20, -54}, {-33, -81}}};

/// Offset of the border indicator flag per nation from the buildings origin
constexpr helpers::EnumArray<std::array<DrawPoint, NUM_MILITARY_BLDS>, Nation> BORDER_FLAG_OFFSET = {
  {{{{-6, -36}, {7, -48}, {-18, -28}, {-47, -64}}},
   {{{17, -45}, {-3, -49}, {-30, -25}, {22, -53}}},
   {{{28, -19}, {29, -18}, {-27, -12}, {-49, -62}}},
   {{{24, -19}, {24, -19}, {17, -52}, {-37, -32}}},
   {{{8, -26}, {13, -36}, {-1, -59}, {-10, -61}}}}};

/// maximale Hitpoints der Soldaten von jedem Volk
constexpr std::array<uint8_t, NUM_SOLDIER_RANKS> HITPOINTS = {3, 4, 5, 6, 7};

/// Max distance for an attacker to reach a building and join in capturing
constexpr unsigned MAX_FAR_AWAY_CAPTURING_DISTANCE = 15;

/// Sichtweite der Militärgebäude (relativ); wird auf die normale Grenzweite draufaddiert
constexpr unsigned VISUALRANGE_MILITARY = 3;
/// Sichtweite von Spähtürmen (absolut)
constexpr unsigned VISUALRANGE_LOOKOUTTOWER = 20;
/// Sichtweite von Spähern
constexpr unsigned VISUALRANGE_SCOUT = 3;
/// Sichtweite von Soldaten
constexpr unsigned VISUALRANGE_SOLDIER = 2;
/// Sichtweite von Schiffen
constexpr unsigned VISUALRANGE_SHIP = 2;
/// Sichtweite von Erkundungs-Schiffen
constexpr unsigned VISUALRANGE_EXPLORATION_SHIP = 12;

/// Beförderungszeit von Soldaten ( =UPGRADE_TIME + rand(UPGRADE_TIME_RANDOM) )
constexpr unsigned UPGRADE_TIME = 100;
constexpr unsigned UPGRADE_TIME_RANDOM = 300;
/// Genesungszeit von Soldaten in Häusern, Zeit, die gebraucht wird um sich um einen Hitpoint zu erholen
// ( =CONVALESCE_TIME + rand(CONVALESCE_TIME_RANDOM) )
constexpr unsigned CONVALESCE_TIME = 500;
constexpr unsigned CONVALESCE_TIME_RANDOM = 500;

/// Maximale Entfernung des Militärgebäudes von dem Hafen bei Seeangriffen
constexpr unsigned SEAATTACK_DISTANCE = 15;

/// Kampfanimationskonstanten für einen Soldatenrang (Gespeichert werden jeweils die IDs in der ROM_BOBS.LST!)
struct FightAnimation
{
    // Angreifen (8 Frames)
    std::array<uint16_t, 8> attacking;
    // 3xVerteidigen mit jeweils 8 Frames
    uint16_t defending[3][8];
};

/// Diese gibts für alle beiden Richtung, für alle 5 Ränge und jeweils nochmal für alle 4 Völker
extern const helpers::EnumArray<helpers::MultiArray<FightAnimation, NUM_SOLDIER_RANKS, 2>, Nation> FIGHT_ANIMATIONS;

/// IDs für die getroffenen (aufleuchtenden) Soldaten für jedes Volk
extern const helpers::EnumArray<std::array<uint16_t, NUM_SOLDIER_RANKS>, Nation> HIT_SOLDIERS;

/// Bestimmt den Aufblinkframe vom den Opfern der folgenden Angreifer (nach Rängen)
constexpr std::array<uint16_t, NUM_SOLDIER_RANKS> SUPPRESS_UNUSED HIT_MOMENT = {{4, 4, 4, 4, 6}};
