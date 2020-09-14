// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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
    enum State
    {
        STATE_WALKING = 0,                  /// Läuft dumm in der Gegend rum
        STATE_PAUSED,                       /// macht mal ne kurze Verschnaufpause ;)
        STATE_WAITINGFORHUNTER,             /// wartet auf den Jäger bis er es abknallt
        STATE_WALKINGUNTILWAITINGFORHUNTER, /// läuft weiter, wartet aber dann auf den Jäger
        STATE_DEAD,                         /// wurde erschossen und liegt tot rum
        STATE_DISAPPEARING                  /// Leiche verschwindet langsam...
    } state;

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

    /// Serialisierungsfunktionen
protected:
    void Serialize_noAnimal(SerializedGameData& sgd) const;

public:
    void Serialize(SerializedGameData& sgd) const override { Serialize_noAnimal(sgd); }

    /// Aufräummethoden
protected:
    void Destroy_noAnimal()
    {
        RTTR_Assert(!hunter);
        noMovable::Destroy();
    }

public:
    void Destroy() override { Destroy_noAnimal(); }

    GO_Type GetGOT() const override { return GOT_ANIMAL; }
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
    bool IsReadyForShooting() const { return (state == STATE_WAITINGFORHUNTER); }
    bool IsGettingReadyForShooting() const { return (state == STATE_WALKINGUNTILWAITINGFORHUNTER); }
    /// Tier soll sterben - erstmal nur Leiche liegen lassen
    void Die();
    /// Tier wurde vom Jäger ausgenommen und muss sofort verschwinden
    void Eviscerated();
};
