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

#include "MapConsts.h"

constexpr unsigned MAX_OBSERVATION_WINDOWS = 50;

enum GUI_ID
{
    CGI_ACTION = 0,
    CGI_CHAT,
    CGI_DIRECTIPCONNECT,
    CGI_DIRECTIPCREATE,
    CGI_ENDGAME,
    CGI_LOBBYCONNECT,
    CGI_LOBBYADDSERVER,
    CGI_LOBBYRANKING,
    CGI_MAINSELECTION,
    CGI_MSGBOX,
    CGI_OPTIONSWINDOW,
    CGI_POSTOFFICE,
    CGI_ROADWINDOW,
    CGI_LOBBYSERVERINFO,
    CGI_README,
    CGI_DISTRIBUTION,
    CGI_BUILDORDER,
    CGI_TRANSPORT,
    CGI_PLAYREPLAY,
    CGI_MILITARY,
    CGI_TOOLS,
    CGI_SKIPGFS,
    CGI_INVENTORY,
    CGI_SAVE,
    CGI_PLEASEWAIT,
    CGI_MINIMAP,
    CGI_BUILDINGS,
    CGI_BUILDINGSPRODUCTIVITY,
    CGI_MUSICPLAYER,
    CGI_INPUTWINDOW,
    CGI_STATISTICS,
    CGI_ECONOMICPROGRESS,
    CGI_DIPLOMACY,
    CGI_SUGGESTPACT,
    CGI_SHIP,
    CGI_HELP,
    CGI_SETTINGS,
    CGI_ADDONS,
    CGI_MERCHANDISE_STATISTICS,
    CGI_MISSION_STATEMENT,
    CGI_MAP_DEBUG,
    CGI_AI_DEBUG,
    CGI_MAP_GENERATOR,
    CGI_VICTORY,
    CGI_OBSERVATION,
    CGI_BUILDING, /// Building windows use this as the base ID and add a unique number for each building
    CGI_NEXT = CGI_BUILDING + MAX_MAP_SIZE * MAX_MAP_SIZE
};
