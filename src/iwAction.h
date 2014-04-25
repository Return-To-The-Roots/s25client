// $Id: iwAction.h 9357 2014-04-25 15:35:25Z FloSoft $
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
#ifndef iwACTION_H_INCLUDED
#define iwACTION_H_INCLUDED

#pragma once

#include "IngameWindow.h"
#include "ctrlGroup.h"

class dskGameInterface;
class GameWorldViewer;

class iwAction : public IngameWindow
{
    public:

        /// Konstanten für ActionWindow-Flag-Tab - Typen
        enum
        {
            AWFT_NORMAL = 0,
            AWFT_HQ,         ///< von der HQ-Flagge kann nur eine Straße gebaut werden
            AWFT_STOREHOUSE, ///< von einer Lagerhaus-Flagge kann nur eine Straße gebaut werden oder die Flagge abgerissen werden
            AWFT_WATERFLAG   ///< Flagge mit Anker drauf (Wasserstraße kann gebaut werden)
        };

        /// Struktur mit den Tabs, die angestellt werden sollen
        class Tabs
        {
            public:

                /// Haupttabs
                bool build, setflag, watch, flag, cutroad, attack, sea_attack;
                /// Gebäude-Bau-Tabs
                enum BuildTab { BT_HUT = 0, BT_HOUSE, BT_CASTLE, BT_MINE, BT_HARBOR } build_tabs;

                Tabs() : build(false), setflag(false), watch(false), flag(false), cutroad(false), attack(false), sea_attack(false),
                    build_tabs(BT_HUT) {}
        };

    private:

        dskGameInterface* const gi;
        GameWorldViewer* const gwv;

        unsigned short selected_x;
        unsigned short selected_y;
        unsigned short last_x;
        unsigned short last_y;

        /// Anzahl gewählter Soldaten für den Angriff und die Maximalanzahl
        unsigned int selected_soldiers_count;
        unsigned int available_soldiers_count;
        /// Dasselbe für Schiffsangriffe
        unsigned selected_soldiers_count_sea;
        unsigned int available_soldiers_count_sea;
        /// Die einzelnen Höhen für die einzelnen Tabs im Bautab
        unsigned short building_tab_heights[4];

    public:
        iwAction(dskGameInterface* const gi, GameWorldViewer* const gwv, const Tabs& tabs, unsigned short selected_x, unsigned short selected_y, int mouse_x, int mouse_y, unsigned int params, bool military_buildings);
        ~iwAction();

        /// Gibt zurück, auf welchen Punkt es sich bezieht
        unsigned short GetSelectedX() const { return selected_x; }
        unsigned short GetSelectedY() const { return selected_y; }

    private:

        void Msg_Group_ButtonClick(const unsigned int group_id, const unsigned int ctrl_id);
        void Msg_TabChange(const unsigned int ctrl_id, const unsigned short tab_id);
        void Msg_Group_TabChange(const unsigned group_id, const unsigned int ctrl_id, const unsigned short tab_id);
        void Msg_PaintAfter();

        inline void Msg_ButtonClick_TabBuild(const unsigned int ctrl_id);
        inline void Msg_ButtonClick_TabCutRoad(const unsigned int ctrl_id);
        inline void Msg_ButtonClick_TabFlag(const unsigned int ctrl_id);
        inline void Msg_ButtonClick_TabAttack(const unsigned int ctrl_id);
        inline void Msg_ButtonClick_TabSeaAttack(const unsigned int ctrl_id);
        inline void Msg_ButtonClick_TabSetFlag(const unsigned int ctrl_id);
        inline void Msg_ButtonClick_TabWatch(const unsigned int ctrl_id);

        /// Fügt Angriffs-Steuerelemente für bestimmte Gruppe hinzu
        void AddAttackControls(ctrlGroup* group, const unsigned attackers_count);
        void AddUpgradeRoad(ctrlGroup* group, unsigned int& x, unsigned int& width);
        void DoUpgradeRoad();

};

#endif // !iwACTION_H_INCLUDED


