// $Id: BinaryFile.cpp 6582 2010-07-16 11:23:35Z FloSoft $
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
#include "BinaryFile.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
	#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
	#undef THIS_FILE
	static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *  @author OLiver
 */
bool BinaryFile::Open(const char * const filename, const OpenFileMode of)
{
	static const char * modes[] = {"wb","rb"};
	return ((file = fopen(filename,modes[of])) ? true : false);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *  @author OLiver
 */
bool BinaryFile::Close()
{
	bool result = false;

	if(file)
		result = (fclose(file)==0);

	file = 0;

	return result;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *  @author OLiver
 */
void BinaryFile::WriteShortString(const std::string& str)
{
	assert(str.length() < 255);
	unsigned char length = static_cast<unsigned char>(str.length())+1;
	WriteUnsignedChar(length);
	WriteRawData((unsigned char*)str.c_str(),length);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *  @author OLiver
 */
void BinaryFile::WriteLongString(const std::string& str)
{
	unsigned length = unsigned(str.length())+1;
	WriteUnsignedInt(length);
	WriteRawData((unsigned char*)str.c_str(),length);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *  @author OLiver
 */
void BinaryFile::ReadShortString(std::string& str)
{
	unsigned char length;
	length = ReadUnsignedChar();
	char * tmp = new char[length];
	ReadRawData((unsigned char*)tmp,length);
	str = tmp;
	delete [] tmp;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *  @author OLiver
 */
void BinaryFile::ReadLongString(std::string& str)
{
	unsigned length;
	length = ReadUnsignedInt();
	char * tmp = new char[length];
	ReadRawData((unsigned char*)tmp,length);
	str = tmp;
	delete [] tmp;
}
