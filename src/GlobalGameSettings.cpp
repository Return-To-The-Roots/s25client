// $Id: GlobalGameSettings.cpp 7084 2011-03-26 21:31:12Z OLiver $
//
// Copyright (c) 2005 - 2010 Settlers Freaks (sf-team at siedler25.org)
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

///////////////////////////////////////////////////////////////////////////////
// Header

#include "stdafx.h"
#include "main.h"
#include "GlobalGameSettings.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
	#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
	#undef THIS_FILE
	static char THIS_FILE[] = __FILE__;
#endif

void GlobalGameSettings::Serialize(Serializer * ser) const
{
	static unsigned char unused;

	ser->PushUnsignedChar(static_cast<unsigned char>(game_speed));
	ser->PushUnsignedChar(static_cast<unsigned char>(game_objective));
	ser->PushUnsignedChar(static_cast<unsigned char>(start_wares));
	ser->PushBool(lock_teams);
	ser->PushUnsignedChar(static_cast<unsigned char>(exploration));
	ser->PushBool(team_view);

	ser->PushUnsignedChar(unused); // old demolition prevention, to not invalidate old savegames
}

void GlobalGameSettings::Deserialize(Serializer * ser)
{
	game_speed = static_cast<GameSpeed>(ser->PopUnsignedChar());
	game_objective = static_cast<GameObjective>(ser->PopUnsignedChar());
	start_wares = static_cast<StartWares>(ser->PopUnsignedChar());
	lock_teams = ser->PopBool();
	exploration = static_cast<Exploration>(ser->PopUnsignedChar());
	team_view = ser->PopBool();

	ser->PopUnsignedChar(); // old demolition prevention, to not invalidate old savegames
}
