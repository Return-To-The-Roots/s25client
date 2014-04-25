// $Id: nofWorkman.h 9357 2014-04-25 15:35:25Z FloSoft $
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

#ifndef NOF_WORKMAN_H_
#define NOF_WORKMAN_H_

#include "nofBuildingWorker.h"

/// Ein Handwerker ist jemand, der seine Waren nur im Gebäude herstellt und diese rausträgt
/// Allen gemeinsam ist, dass sie einen festen Arbeitsablauf haben:
/// Warten -- Arbeiten -- Warten -- Ware raustragen -- wieder reinkommen -- ...
class nofWorkman : public nofBuildingWorker
{
    private:

        // Funktionen, die nur von der Basisklasse  aufgerufen werden, wenn...
        virtual void WalkedDerived(); // man gelaufen ist
        /// Gibt den Warentyp zurück, welche der Arbeiter erzeugen will
        virtual GoodType ProduceWare() = 0;
        /// Abgeleitete Klasse informieren, wenn man fertig ist mit Arbeiten
        virtual void WorkFinished();

    protected:

        /// Entsprechende Methoden für die Abwicklung der einzelnen Zustände
        /// Nach erstem Warten, sprich der Arbeiter muss versuchen, neu anfangen zu arbeiten
        void HandleStateWaiting1();
        void HandleStateWaiting2();
        void HandleStateWork();

    public:

        nofWorkman(const Job job, const unsigned short x, const unsigned short y, const unsigned char player, nobUsual* workplace);
        nofWorkman(SerializedGameData* sgd, const unsigned obj_id);

        /// Serialisierungsfunktionen
    protected:  void Serialize_nofWorkman(SerializedGameData* sgd) const;
    public:     void Serialize(SerializedGameData* sgd) const { Serialize_nofWorkman(sgd); }

        virtual void HandleDerivedEvent(const unsigned int id);


};

#endif
