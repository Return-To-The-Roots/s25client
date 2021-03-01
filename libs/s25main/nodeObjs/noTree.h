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

#include "noCoordBase.h"

class FOWObject;
class SerializedGameData;
class GameEvent;

/// Stellt einen Baum dar
/*

Bäume können ausgewachsen sein oder noch wachsen in 3 Schritten, das gibt size an.
Wenn ein Baum gefällt wird, wird er vom Holzfäller gezeichnet.
Das Wachsen von Bäumen sieht so aus:

_______________|__|_________________|_| ...
  Warten      Wachsen   Warten    Wachsen

Es wird eine längere Zeit gewartet, dann wächst der Baum, dabei wird die nächste Stufe langsam eingeblendet.

 */
class noTree : public noCoordBase
{
    /// Der Holzfäller ist Experte in Sachen Baum :)
    // friend class nofWoodcutter;
    // friend class AIPlayerJH;
    // friend class AIInterface;

    /// Typ des Baumes (also welche Baumart)
    unsigned char type;
    /// Größe des Baumes (0-2, 3 = aufgewachsen!)
    unsigned char size;
    enum class State : uint8_t
    {
        Nothing,      // Baum steht einfach nur rum
        GrowingWait,  // Baum ist noch im Wachsstadium, hat aber gerade keinen "Wachstumsschub"
        GrowingGrow,  // Baum hat gerade den Wachstumsschub
        FallingWait,  // Baum wartet noch bis er umfällt
        FallingFall,  // Baum fällt gerade um
        FallingFallen // Baum ist schon umgefallen und liegt noch ne Weile da bis er abtransportiert wird
    } state;
    friend constexpr auto maxEnumValue(State) { return State::FallingFallen; }

    /// Wachsevent
    const GameEvent* event;
    /// Tier-Produier-Event
    const GameEvent* produce_animal_event;

    /// Produziert dieser Baum Tiere?
    bool produce_animals;
    /// Zählt gezeichnete Bäume innerhalb eines Zeichenvorgangs (so und soviel Vogelgezwitscher muss dann abgespielt
    /// werden)
    static unsigned short DRAW_COUNTER;

    /// Gibt Warte- und Wachsdauer der Bäume an
    static const unsigned WAIT_LENGTH = 835;
    static const unsigned GROWING_LENGTH = 15;

private:
    /// "Produziert" ein Tier
    void ProduceAnimal();

public:
    noTree(MapPoint pos, unsigned char type, unsigned char size);
    noTree(SerializedGameData& sgd, unsigned obj_id);

    ~noTree() override;

    void Destroy() override;
    void Serialize(SerializedGameData& sgd) const override;

    GO_Type GetGOT() const final { return GO_Type::Tree; }

    void Draw(DrawPoint drawPt) override;

    void HandleEvent(unsigned id) override;

    BlockingManner GetBM() const override { return BlockingManner::Tree; }

    /// Erzeugt von ihnen selbst ein FOW Objekt als visuelle "Erinnerung" für den Fog of War
    std::unique_ptr<FOWObject> CreateFOWObject() const override;
    /// Can this tree(type) produce wood?
    bool ProducesWood() const { return type != 5; }
    /// Return if this tree is fully grown
    bool IsFullyGrown() const { return size == 3; }

    /// Vom Holzfäller aufgerufen: Wenn der Holzfäller anfängt und der Baum dann in einer bestimmten Zeit umfallen soll
    void FallSoon();
    /// Vom Holzfäller aufgerufen: Der Holzfäller hat seine Arbeit unterbrochen und der Baum soll doch nicht umfallen
    void DontFall();

    /// Setzt Draw-Counter auf 0
    static void ResetDrawCounter() { DRAW_COUNTER = 0; }
    /// Liest Draw-Counter aus
    static unsigned short QueryDrawCounter() { return DRAW_COUNTER; }
};
