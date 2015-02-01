// $Id: iwBuildings.h 9595 2015-02-01 09:40:54Z marcus $
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
#ifndef iwBUILDINGS_H_
#define iwBUILDINGS_H_


#include "IngameWindow.h"
class dskGameInterface;
class GameWorldViewer;

/// Fenster, welches die Anzahl aller Gebäude und der Baustellena auflistet
class iwBuildings : public IngameWindow
{
	GameWorldViewer* const gwv;
    dskGameInterface* const gi;
    public:

        /// Konstruktor von @p iwMilitary.
        iwBuildings(GameWorldViewer* const gwv, dskGameInterface* const gi);

    private:

        /// Anzahlen der Gebäude zeichnen
        void Msg_PaintAfter();
		
		void Msg_ButtonClick(const unsigned int ctrl_id);

};



#endif
