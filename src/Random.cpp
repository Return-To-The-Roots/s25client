// $Id: Random.cpp 6582 2010-07-16 11:23:35Z FloSoft $
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
#include "main.h"
#include "Random.h"

#include "GameClient.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
	#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
	#undef THIS_FILE
	static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////
/**
 *  Konstruktor von @p Random.
 *
 *  @author FloSoft
 */
Random::Random(void)
{
	/*log = fopen("async_log.txt","w");*/
	Init(123456789);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Initialisiert den Zufallszahlengenerator.
 *
 *  @param[in] init Zahl mit der der Zufallsgenerator initialisiert wird.
 *
 *  @author OLiver
 */
void Random::Init(const unsigned int init)
{
	zahl = init;
	counter = 0;
	async_log.clear();
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Erzeugt eine Zufallszahl.
 *
 *  @param[in] max @p max-1 ist die maximale Zufallszahl welche geliefert werden soll.
 *
 *  @return liefert eine Zufallszahl.
 *
 *  @author OLiver
 */
int Random::Rand(const char * const src_name, const unsigned src_line,const unsigned obj_id,const int max)
{
	zahl = ( (zahl * 997) + 1+max) & 32767;

	/// Randomgenerator loggen
	char log_str[512];
	//assert(counter != 7731970); <- wtf?
	sprintf(log_str,"%u:R(%d)=%d,z=%d | %s Z: %u|id=%u",counter,max,(zahl * max) / 32768,zahl,src_name,src_line,obj_id);
	async_log.push_back(log_str);
	if(async_log.size() > 10000)
		async_log.pop_front();

	++counter;
	return ( (zahl * max) / 32768);
}


void Random::SaveLog(const char * const filename)
{
	FILE * file = fopen(filename,"w");

	for(list<std::string>::iterator it = async_log.begin();it.valid();++it)
		fprintf(file,"%s\n",it->c_str());

	fclose(file);
}
