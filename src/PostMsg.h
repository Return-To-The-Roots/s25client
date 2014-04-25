// $Id: PostMsg.h jh
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
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

#ifndef POSTMSG_H_
#define POSTMSG_H_

#include "main.h"
#include <string>
#include "GameConsts.h"
#include "MapConsts.h"
#include "Loader.h"

class SerializedGameData;

/// Einfache Post-Nachricht, nur mit Text.
class PostMsg
{
    public:
        PostMsg(const std::string& text, PostMessageCategory cat);
        PostMsg(SerializedGameData* sgd);
        virtual ~PostMsg();

        const std::string& GetText() const { return text; }
        PostMessageType GetType() const { return type; }
        PostMessageCategory GetCategory() const { return cat; }
        unsigned GetSendFrame() const { return sendFrame; }
        virtual void Serialize(SerializedGameData* sgd);

    protected:
        std::string text;
        PostMessageType type;
        PostMessageCategory cat;
        unsigned sendFrame;
};

/// Post-Nachricht mit Text und einem Goto-Knopf der zu einem bestimmten Kartenpunkt führt
class PostMsgWithLocation : public PostMsg
{
    public:
        PostMsgWithLocation(const std::string& text, PostMessageCategory cat, MapCoord x, MapCoord y);
        PostMsgWithLocation(SerializedGameData* sgd);

        MapCoord GetX() const { return x; }
        MapCoord GetY() const { return y; }
        virtual void Serialize(SerializedGameData* sgd);

    private:
        MapCoord x;
        MapCoord y;
};

/// Post-Nachricht mit Bild, Text und Goto-Button
class ImagePostMsgWithLocation : public PostMsgWithLocation
{
    public:
        ImagePostMsgWithLocation(const std::string& text, PostMessageCategory cat, MapCoord x, MapCoord y, BuildingType senderBuilding, Nation senderNation);
        ImagePostMsgWithLocation(const std::string& text, PostMessageCategory cat, MapCoord x, MapCoord y, Nation senderNation);
        ImagePostMsgWithLocation(SerializedGameData* sgd);

        glArchivItem_Bitmap* GetImage_() const;
        virtual void Serialize(SerializedGameData* sgd);

    private:
        BuildingType senderBuilding;
        Nation senderNation;
};

class iwPostWindow;
// TODO: evtl noch verschiedene ermöglichen durch einen weiteren Parameter? Allianz, Nicht-Angriffspakt, Zeitbegrenzung, whatever
/// Diplomatie-Post-Nachricht, mit Annehmen- und Ablehnen-Knopf
class DiplomacyPostQuestion : public PostMsg
{
        friend class iwPostWindow;
    public:
        /// Typ der Diplomatienachricht
        enum Type
        {
            ACCEPT, /// Nachricht, die den Spieler fragt, ob ein anderer Spieler den Vertrag akzeptiert
            CANCEL /// Nachricht, die den Spieler fragt, ob ein bestehender Vertrag aufgelöst werden soll
        };

        /// Vertrag akzeptieren
        DiplomacyPostQuestion(const unsigned id, const unsigned char player, const PactType pt, const unsigned duration);
        /// Vertrag auflösen
        DiplomacyPostQuestion(const unsigned id, const unsigned char player, const PactType pt);
        DiplomacyPostQuestion(SerializedGameData* sgd);

        unsigned GetPlayerID() const { return player; }
        virtual void Serialize(SerializedGameData* sgd);

    private:
        /// Typ der Diplomatienachricht
        Type dp_type;

        /// ID des Vertrages (= normalerweise die GF-Nummer, zu der es vorgeschlagen wurde)
        unsigned id;
        /// Spieler, den das Bündnis betrifft
        unsigned char player;
        /// Vertragsart
        PactType pt;
};

///
class DiplomacyPostInfo : public PostMsg
{
        friend class iwPostWindow;
    public:
        enum Type
        {
            ACCEPT, /// Nachricht, die den Spieler fragt, ob ein anderer Spieler den Vertrag akzeptiert
            CANCEL /// Nachricht, die den Spieler fragt, ob ein bestehender Vertrag aufgelöst werden soll
        };

        DiplomacyPostInfo(const unsigned char other_player, const Type dp_type, const PactType pt);
        DiplomacyPostInfo(SerializedGameData* sgd);

        virtual void Serialize(SerializedGameData* sgd);

        /// Typ der Diplomatienachricht

};

class noShip;

class ShipPostMsg : public ImagePostMsgWithLocation
{
    public:
        ShipPostMsg(const std::string& text, PostMessageCategory cat, Nation senderNation, MapCoord x, MapCoord y);
        glArchivItem_Bitmap* GetImage_() const;
        MapCoord GetX() const;
        MapCoord GetY() const;

    private:
        noShip* ship;

};

#endif
