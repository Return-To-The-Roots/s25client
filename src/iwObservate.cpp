// $Id: iwDemolishBuilding.cpp 7091 2011-03-27 10:57:38Z OLiver $
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
#include <stdafx.h>
#include "main.h"
#include "iwObservate.h"

#include "Loader.h"
#include "VideoDriverWrapper.h"
#include "GameClient.h"
#include "WindowManager.h"
#include "GameWorld.h"
#include "iwMsgbox.h"
#include "GameCommands.h"
#include "dskGameInterface.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
	#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
	#undef THIS_FILE
	static char THIS_FILE[] = __FILE__;
#endif
//IngameWindow::IngameWindow(unsigned int id, unsigned short x, unsigned short y, unsigned short width, unsigned short height, const std::string& title, glArchivItem_Bitmap *background, bool modal)
iwObservate::iwObservate(GameWorldViewer * const gwv,const unsigned short selected_x, const unsigned short selected_y)
: IngameWindow(gwv->CreateGUIID(selected_x, selected_y),0xFFFE,0xFFFE,300,300,_("Observation window"),NULL),
	view(new GameWorldView(gwv, GetX(), GetY(), 300, 300)), selected_x(selected_x), selected_y(selected_y)
{
	view->MoveToMapObject(selected_x, selected_y);
}

bool iwObservate::Draw_()
{
	if ((x != last_x) || (y != last_y))
	{
		view->SetX(GetX());
		view->SetY(GetY());
		last_x = x;
		last_y = y;
		view->terrain_rerender = true;
	}

	glTranslatef(0.0f, 0.0f, 9.9f);
	DrawRectangle(x, y, width, height, COLOR_WHITE);
	glTranslatef(0.0f, 0.0f, -9.9f);

	RoadsBuilding road;

	road.mode = RM_DISABLED;
	road.point_x = 0;
	road.point_y = 0;
	road.start_x = 0;
	road.start_y = 0;

	unsigned water;
	view->Draw(GAMECLIENT.GetPlayerID(), &water, true, selected_x, selected_y, road);

	return(IngameWindow::Draw_());
}

