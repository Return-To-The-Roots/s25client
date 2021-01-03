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

#include "figures/nofSoldier.h"
#include "nobBaseMilitary.h"
#include <list>
#include <vector>

class nofPassiveSoldier;
class nofActiveSoldier;
class nofAttacker;
class nofAggressiveDefender;
class nofDefender;
class Ware;
class SerializedGameData;
class noFigure;
class GameEvent;

/// Distance to the next enemy border
enum class FrontierDistance : uint8_t
{
    Far,    /// next military building is far away
    Mid,    /// Next military building is in reachable range
    Harbor, /// Military building is near a harbor
    Near    /// Military building is next to a border
};
constexpr auto maxEnumValue(FrontierDistance)
{
    return FrontierDistance::Near;
}

/// Stellt ein Militärgebäude beliebiger Größe (also von Baracke bis Festung) dar
class nobMilitary : public nobBaseMilitary
{
public:
private:
    /// wurde das Gebäude gerade neu gebaut (muss also die Landgrenze beim Eintreffen von einem Soldaten neu berechnet
    /// werden?)
    bool new_built;
    /// Anzahl der Goldmünzen im Gebäude
    unsigned char numCoins;
    /// Gibt an, ob Goldmünzen gesperrt worden (letzteres nur visuell, um Netzwerk-Latenzen zu verstecken)
    bool coinsDisabled, coinsDisabledVirtual;
    /// Entfernung zur freindlichen Grenze (woraus sich dann die Besatzung ergibt) von 0-3, 0 fern, 3 nah, 2 Hafen!
    FrontierDistance frontier_distance;
    /// Größe bzw Typ des Militärgebäudes (0 = Baracke, 3 = Festung)
    unsigned char size;
    /// Bestellte Soldaten
    SortedTroops ordered_troops;
    /// Bestellter Goldmünzen
    std::list<Ware*> ordered_coins;
    /// Gibt an, ob gerade die Eroberer in das Gebäude gehen (und es so nicht angegegriffen werden sollte)
    bool capturing;
    /// Anzahl der Soldaten, die das Militärgebäude gerade noch einnehmen
    unsigned capturing_soldiers;
    /// List of soldiers who are on the way to capture the military building
    /// but who are still quite far away (didn't stand around the building)
    std::list<nofAttacker*> far_away_capturers;
    /// Gold-Bestell-Event
    const GameEvent* goldorder_event;
    /// Beförderung-Event
    const GameEvent* upgrade_event;
    /// Is the military building regulating its troops at the moment? (then block furthere RegulateTroop calls)
    bool is_regulating_troops;
    /// Soldatenbesatzung
    SortedTroops troops;

    /// Bestellungen (sowohl Truppen als auch Goldmünzen) zurücknehmen
    void CancelOrders();
    /// Wählt je nach Militäreinstellungen (Verteidigerstärke) einen passenden Soldaten aus
    nofPassiveSoldier* ChooseSoldier();
    /// Stellt Verteidiger zur Verfügung
    nofDefender* ProvideDefender(nofAttacker* attacker) override;
    /// Will/kann das Gebäude noch Münzen bekommen?
    bool WantCoins() const;
    /// Prüft, ob Goldmünzen und Soldaten, die befördert werden können, vorhanden sind und meldet ggf. ein
    /// Beförderungsevent an
    void PrepareUpgrading();
    /// Gets the total amount of soldiers (ordered, stationed, on mission)
    size_t GetTotalSoldiers() const;
    /// Looks for the next far-away-capturer waiting around and calls it to the flag
    void CallNextFarAwayCapturer(nofAttacker* attacker);

    friend class SerializedGameData;
    friend class BuildingFactory;
    nobMilitary(BuildingType type, MapPoint pos, unsigned char player, Nation nation);
    nobMilitary(SerializedGameData& sgd, unsigned obj_id);

public:
    ~nobMilitary() override;

protected:
    void DestroyBuilding() override;
    void Serialize_nobMilitary(SerializedGameData& sgd) const;

public:
    void Serialize(SerializedGameData& sgd) const override { Serialize_nobMilitary(sgd); }

    GO_Type GetGOT() const override { return GO_Type::NobMilitary; }

    void Draw(DrawPoint drawPt) override;
    void HandleEvent(unsigned id) override;

    /// Wurde das Militärgebäude neu gebaut und noch nicht besetzt und kann somit abgerissen werden bei Land-verlust?
    bool IsNewBuilt() const { return new_built; }

    /// Liefert Militärradius des Gebäudes
    unsigned GetMilitaryRadius() const override;
    unsigned GetMaxCoinCt() const;
    unsigned GetMaxTroopsCt() const;

    /// Sucht feindliche Miitärgebäude im Umkreis und setzt die frontier_distance entsprechend (sowohl selber als
    /// auch von den feindlichen Gebäuden) und bestellt somit ggf. neue Soldaten, exception wird nicht mit einbezogen
    void LookForEnemyBuildings(const nobBaseMilitary* exception = nullptr);

    /// Wird von gegnerischem Gebäude aufgerufen, wenn sie neu gebaut worden sind und es so ein neues Gebäude im Umkreis
    /// gibt setzt frontier_distance neu falls möglich und sendet ggf. Verstärkung
    void NewEnemyMilitaryBuilding(FrontierDistance distance);
    bool IsUseless() const;
    bool IsAttackable(unsigned playerIdx) const override;
    /// Gibt Distanz zurück
    FrontierDistance GetFrontierDistance() const { return frontier_distance; }

    /// Berechnet die gewünschte Besatzung je nach Grenznähe
    unsigned CalcRequiredNumTroops() const;
    /// Calculate the required troop count for the given setting
    unsigned CalcRequiredNumTroops(FrontierDistance assumedFrontierDistance, unsigned settingValue) const;
    /// Reguliert die Besatzung des Gebäudes je nach Grenznähe, bestellt neue Soldaten und schickt überflüssige raus
    void RegulateTroops();
    /// Gibt aktuelle Besetzung zurück
    unsigned GetNumTroops() const { return troops.size(); }
    const SortedTroops& GetTroops() const { return troops; }

    /// Wird aufgerufen, wenn eine neue Ware zum dem Gebäude geliefert wird (in dem Fall nur Goldstücke)
    void TakeWare(Ware* ware) override;
    /// Legt eine Ware am Objekt ab (an allen Straßenknoten (Gebäude, Baustellen und Flaggen) kann man Waren ablegen
    void AddWare(Ware*& ware) override;
    /// Eine bestellte Ware konnte doch nicht kommen
    void WareLost(Ware* ware) override;
    /// Wird aufgerufen, wenn von der Fahne vor dem Gebäude ein Rohstoff aufgenommen wurde
    bool FreePlaceAtFlag() override;

    /// Berechnet, wie dringend eine Goldmünze gebraucht wird, in Punkten, je höher desto dringender
    unsigned CalcCoinsPoints() const;

    /// Wird aufgerufen, wenn ein Soldat kommt
    void GotWorker(Job job, noFigure* worker) override;
    /// Fügt aktiven Soldaten (der aus von einer Mission) zum Militärgebäude hinzu
    void AddActiveSoldier(nofActiveSoldier* soldier) override;
    /// Fügt passiven Soldaten (der aus einem Lagerhaus kommt) zum Militärgebäude hinzu
    void AddPassiveSoldier(nofPassiveSoldier* soldier);
    /// Soldat konnte nicht kommen
    void SoldierLost(nofSoldier* soldier) override;
    /// Soldat ist jetzt auf Mission
    void SoldierOnMission(nofPassiveSoldier* passive_soldier, nofActiveSoldier* active_soldier);

    /// Schickt einen Verteidiger raus, der einem Angreifer in den Weg rennt
    nofAggressiveDefender* SendAggressiveDefender(nofAttacker* attacker) override;

    /// Gibt die Anzahl der Soldaten zurück, die für einen Angriff auf ein bestimmtes Ziel zur Verfügung stehen
    unsigned GetNumSoldiersForAttack(MapPoint dest) const;
    /// Gibt die Soldaten zurück, die für einen Angriff auf ein bestimmtes Ziel zur Verfügung stehen
    std::vector<nofPassiveSoldier*> GetSoldiersForAttack(MapPoint dest) const;
    /// Gibt die Stärke der Soldaten zurück, die für einen Angriff auf ein bestimmtes Ziel zur Verfügung stehen
    unsigned GetSoldiersStrengthForAttack(MapPoint dest, unsigned& soldiers_count) const;
    /// Gibt die Stärke eines Militärgebäudes zurück
    unsigned GetSoldiersStrength() const;

    /// Gebäude wird vom Gegner eingenommen, player ist die neue Spieler-ID
    void Capture(unsigned char new_owner);
    /// Das Gebäude wurde bereits eingenommen, hier wird geprüft, ob noch weitere Soldaten für die Besetzung
    /// notwendig sind, wenn ja wird ein neuer Soldat gerufen, wenn nein, werden alle restlichen nach Hause geschickt
    void NeedOccupyingTroops();
    /// Sagt dem Gebäude schonmal, dass es eingenommen wird, wenn er erste Eroberer gerade in das Gebäude reinläuft
    /// (also noch bevor er drinnen ist!) - damit da nicht zusätzliche Soldaten reinlaufen
    void PrepareCapturing()
    {
        RTTR_Assert(!IsBeingCaptured());
        capturing = true;
        ++capturing_soldiers;
    }

    /// Wird das Gebäude gerade eingenommen?
    bool IsBeingCaptured() const { return capturing; }
    /// Gebäude wird nicht mehr eingenommen (falls anderer Soldat zuvor reingekommen ist beim Einnehmen)
    void StopCapturing()
    {
        capturing_soldiers = 0;
        capturing = false;
    }
    /// Sagt, dass ein erobernder Soldat das Militärgebäude erreicht hat
    void CapturingSoldierArrived();
    /// A far-away capturer arrived around the building and starts waiting
    void FarAwayCapturerReachedGoal(nofAttacker* attacker);

    bool IsFarAwayCapturer(nofAttacker* attacker);

    /// Stoppt/Erlaubt Goldzufuhr (visuell)
    void ToggleCoinsVirtual() { coinsDisabledVirtual = !coinsDisabledVirtual; }
    /// Stoppt/Erlaubt Goldzufuhr (real)
    void SetCoinsAllowed(bool enabled);
    /// Fragt ab, ob Goldzufuhr ausgeschaltet ist (visuell)
    bool IsGoldDisabledVirtual() const { return coinsDisabledVirtual; }
    /// Fragt ab, ob Goldzufuhr ausgeschaltet ist (real)
    bool IsGoldDisabled() const { return coinsDisabled; }
    unsigned char GetNumCoins() const { return numCoins; }
    /// is there a max rank soldier in the building?
    bool HasMaxRankSoldier() const;

    /// Sucht sämtliche Lagerhäuser nach Goldmünzen ab und bestellt ggf. eine, falls eine gebraucht wird
    void SearchCoins();

    /// Gebäude wird von einem Katapultstein getroffen
    void HitOfCatapultStone();

    /// Sind noch Truppen drinne, die dieses Gebäude verteidigen können
    bool DefendersAvailable() const override { return (GetNumTroops() > 0); }

    /// send all soldiers of the highest rank home (if highest=lowest keep 1)
    void SendSoldiersHome();
    /// order new troops
    void OrderNewSoldiers();

    /// Darf das Militärgebäude abgerissen werden (Abriss-Verbot berücksichtigen)?
    bool IsDemolitionAllowed() const;

    void UnlinkAggressor(nofAttacker* soldier) override;
};
