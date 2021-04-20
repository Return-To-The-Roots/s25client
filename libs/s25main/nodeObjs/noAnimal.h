// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "helpers/OptionalEnum.h"
#include "noMovable.h"
#include "gameTypes/AnimalTypes.h"

class nofHunter;
class SerializedGameData;

/// Klasse für die Tiere (ausgenommen Esel und Schweine natürlich)
class noAnimal : public noMovable
{
    /// Tierart
    Species species;

    /// Was macht das Tier gerade?
    enum class State : uint8_t
    {
        Walking,                      /// Läuft dumm in der Gegend rum
        Paused,                       /// macht mal ne kurze Verschnaufpause ;)
        WaitingForHunter,             /// wartet auf den Jäger bis er es abknallt
        WalkingUntilWaitingForHunter, /// läuft weiter, wartet aber dann auf den Jäger
        Dead,                         /// wurde erschossen und liegt tot rum
        Disappearing                  /// Leiche verschwindet langsam...
    } state;
    friend constexpr auto maxEnumValue(State) { return State::Disappearing; }

    /// Wie weit kann es noch rumlaufen, bis es mal wieder eine Pause machen muss
    unsigned short pause_way;
    /// Jäger, der das Tier jagt (0, falls nicht gejagt)
    nofHunter* hunter;
    /// Nächster Zeitpunkt, ab wann der Sound gespielt werden soll (bei Enten und Schafen)
    unsigned sound_moment;

private:
    /// entscheidet, was nach einem gelaufenen Abschnitt weiter zu tun ist
    void Walked();
    /// Sucht eine Richtung, in die das Tier gehen kann
    helpers::OptionalEnum<Direction> FindDir();
    /// Fängt an zu laufen
    void StartWalking(Direction dir);
    /// Sucht eine neue Richtung und läuft in diese, ansonsten stirbt es
    void StandardWalking();

public:
    noAnimal(Species species, MapPoint pos);
    noAnimal(SerializedGameData& sgd, unsigned obj_id);

    ~noAnimal() override = default;

    void Serialize(SerializedGameData& sgd) const override;
    void Destroy() override
    {
        RTTR_Assert(!hunter);
        noMovable::Destroy();
    }

    GO_Type GetGOT() const final { return GO_Type::Animal; }
    Species GetSpecies() const { return species; }

    // An x,y zeichnen
    void Draw(DrawPoint drawPt) override;
    // Benachrichtigen, wenn neuer gf erreicht wurde
    void HandleEvent(unsigned id) override;

    /// Wird aufgerufen, nachdem das Tier erzeugt wurde und zur Figurenliste hinzugefügt wurde
    void StartLiving();

    /// Kann das Tier noch vom Jäger gejagt werden?
    bool CanHunted() const;

    /// Ein Jäger geht das Tier jagen
    void BeginHunting(nofHunter* hunter);
    /// Ein Jäger ist in der Nähe vom Tier und will es abschießen --> Tier muss stillhalten
    /// gibt die Koordinaten seines (späteren) Standorts zurück
    MapPoint HunterIsNear();
    /// Ein Jäger kann doch nicht mehr zum Tier kommen --> kann wieder normal weiterlaufen
    void StopHunting();

    /// Steht das Tier schon schön ruhig, damit der Jäger es erschießen kann?
    bool IsReadyForShooting() const { return (state == State::WaitingForHunter); }
    bool IsGettingReadyForShooting() const { return (state == State::WalkingUntilWaitingForHunter); }
    /// Tier soll sterben - erstmal nur Leiche liegen lassen
    void Die();
    /// Tier wurde vom Jäger ausgenommen und muss sofort verschwinden
    void Eviscerated();
};
