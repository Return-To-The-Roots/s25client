// $Id: BinaryFile.h 6582 2010-07-16 11:23:35Z FloSoft $
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
#ifndef BINARYFILE_H_INCLUDED
#define BINARYFILE_H_INCLUDED

#pragma once

class String;

// Öffnungsmethoden
enum OpenFileMode
{
	OFM_WRITE = 0, 
	OFM_READ
};

/// Klasse für das Laden und Speichern von binären Dateien
class BinaryFile
{
public:
	BinaryFile() : file(0) {}
	~BinaryFile() { Close(); }

	/// Öffnet eine Datei: liefert true falls erfolgreich
	bool Open(const char *const filename, const OpenFileMode of);
	/// Schließt eine Datei: liefert true falls erfolgreich
	bool Close();

	/// Schreibmethoden
	inline void WriteSignedInt(int i) const
	{
		libendian::le_write_i(i, file);
	}
	inline void WriteUnsignedInt(unsigned int i) const
	{
		libendian::le_write_ui(i, file);
	}
	inline void WriteSignedShort(short i) const
	{
		libendian::le_write_s(i, file);
	}

	inline void WriteUnsignedShort(unsigned short i) const
	{
		libendian::le_write_us(i, file); 
	}
	inline void WriteSignedChar(char i) const
	{
		libendian::le_write_c(&i, 1, file);
	}
	inline void WriteUnsignedChar(unsigned char i) const
	{
		libendian::le_write_uc(&i, 1, file);
	}
	inline void WriteRawData(const void *const data, const unsigned int length) const
	{
		libendian::le_write_c((char*)data, length, file);
	}

	void WriteShortString(const std::string& str); /// Länge max 254
	void WriteLongString(const std::string& str); /// Länge max 2^32-2

	/// Lesemethoden
	inline int ReadSignedInt(void) 
	{ 
		signed int i; 
		libendian::le_read_i(&i, file);
		return i; 
	}
	inline unsigned int ReadUnsignedInt(void)
	{ 
		unsigned int i;
		libendian::le_read_ui(&i, file);
		return i; 
	}
	inline short ReadSignedShort(void)
	{
		signed short i; 
		libendian::le_read_s(&i, file);
		return i;
	}
	inline unsigned short ReadUnsignedShort(void) 
	{
		unsigned short i;
		libendian::le_read_us(&i, file);
		return i;
	}
	inline char ReadSignedChar(void)
	{ 
		char i;
		libendian::le_read_c(&i, 1, file);
		return i; 
	}
	inline unsigned char ReadUnsignedChar(void)
	{
		unsigned char i;
		libendian::le_read_uc(&i, 1, file);
		return i; 
	}
	inline void ReadRawData(void *const data, const unsigned int length)
	{ 
		libendian::le_read_c((char*)data, length, file);
	}

	void ReadShortString(std::string& str); /// Länge max 254
	void ReadLongString(std::string& str); /// Länge max 2^32-2

	/// Setzt den Dateizeiger
	void Seek(const long pos,const int origin)
	{
		fseek(file,pos,origin);
	}

	/// Liefert Den Dateizeiger
	unsigned Tell() const
	{
		return ftell(file);
	}

	/// Schreibt alles richtig in die Datei
	void Flush()
	{
		fflush(file);
	}

	/// Datei zu Ende?
	bool EndOfFile() const
	{
		return feof(file)? true : false;
	}

	/// Datei gültig?
	bool IsValid() const
	{
		return file ? true : false;
	}

private:
	/// File-Pointer
	FILE * file;
};


#endif // !BINARYFILE_H_INCLUDED
