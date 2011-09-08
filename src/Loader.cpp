// $Id: Loader.cpp 7521 2011-09-08 20:45:55Z FloSoft $
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

///////////////////////////////////////////////////////////////////////////////
// Header
#include "main.h"

#include <iomanip>
#include "Loader.h"

#include "files.h"
#include "GlobalVars.h"
#include "Settings.h"

#include "VideoDriverWrapper.h"
#include "AudioDriverWrapper.h"

#include "CollisionDetection.h"
#include "GameClient.h"
#include "GameClientPlayer.h"
#include "ListDir.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
	#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
	#undef THIS_FILE
	static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////
/**
 *  Konstruktor von @p Loader.
 *
 *  @author FloSoft
 */
Loader::Loader(void) : lastgfx(0xFF)
{
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Destruktor von @p Loader.
 *
 *  @author FloSoft
 */
Loader::~Loader(void)
{
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Lädt alle allgemeinen Dateien.
 *
 *  @return @p true bei Erfolg, @p false bei Fehler.
 *
 *  @author FloSoft
 *  @author OLiver
 */
bool Loader::LoadFilesAtStart(void)
{
	const unsigned int files_count = 7 + 1 + 2 + 2 + 21;

	const unsigned int files[] = { 
		5, 6, 7, 8, 9, 10, 17, // Paletten:     pal5.bbm, pal6.bbm, pal7.bbm, paletti0.bbm, paletti1.bbm, paletti8.bbm, colors.act
		FILE_SPLASH_ID,	       // Splashscreen: splash.bmp
		11, 12,                // Menüdateien:  resource.dat, io.dat
		102, 103,              // Hintergründe: setup013.lbm, setup015.lbm
		64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84 // Die ganzen Spielladescreens.
	};

	if(!LoadFilesFromArray(files_count, files))
		return false;

	if(!LoadSounds())
		return false;

	if(!LoadLsts(95)) // lade systemweite und persönliche lst files
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  @brief
 *
 *  @author FloSoft
 */
bool Loader::LoadFileOrDir(const std::string& file, const unsigned int file_id, bool load_always)
{
		// is the entry a directory?
		if(IsDir(file))
		{
			// yes, load all files in the directory
			unsigned int ladezeit = VideoDriverWrapper::inst().GetTickCount();

			LOG.lprintf("Lade LST,BOB,IDX,BMP,TXT,GER,ENG Dateien aus \"%s\"\n", GetFilePath(file).c_str());

			std::list<std::string> lst;
			ListDir(GetFilePath(file) + "*.lst", true, NULL, NULL, &lst);
			ListDir(GetFilePath(file) + "*.bob", true, NULL, NULL, &lst);
			ListDir(GetFilePath(file) + "*.idx", true, NULL, NULL, &lst);
			ListDir(GetFilePath(file) + "*.bmp", true, NULL, NULL, &lst);
			ListDir(GetFilePath(file) + "*.txt", true, NULL, NULL, &lst);
			ListDir(GetFilePath(file) + "*.ger", true, NULL, NULL, &lst);
			ListDir(GetFilePath(file) + "*.eng", true, NULL, NULL, &lst);

			for(std::list<std::string>::iterator i = lst.begin(); i != lst.end(); ++i)
			{
				if(!LoadFile( i->c_str(), GetPaletteN("pal5"), load_always ) )
					return false;
			}
			LOG.lprintf("fertig (%ums)\n", VideoDriverWrapper::inst().GetTickCount()-ladezeit);
		}
		else
		{
			// no, only single file specified
			if(!LoadFile(file.c_str(), GetPaletteN("pal5"), load_always ) )
				return false;

			// ggf Splash anzeigen
			if(file_id == FILE_SPLASH_ID)
			{
				glArchivItem_Bitmap *image = GetImageN("splash", 0);
				image->setFilter(GL_LINEAR);
				image->Draw(0, 0, VideoDriverWrapper::inst().GetScreenWidth(), VideoDriverWrapper::inst().GetScreenHeight(), 0, 0, 0, 0);
				VideoDriverWrapper::inst().SwapBuffers();
			}
		}
		return true;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Lädt Dateien aus FILE_PATHS bzw aus dem Verzeichnis.
 *
 *  @return @p true bei Erfolg, @p false bei Fehler.
 *
 *  @author FloSoft
 */
bool Loader::LoadFilesFromArray(const unsigned int files_count, const unsigned int *files, bool load_always)
{
	// load the files or directorys
	for(unsigned int i = 0; i < files_count; ++i)
	{
		if(files[i] == 0xFFFFFFFF)
			continue;

		if(!LoadFileOrDir(FILE_PATHS[ files[i] ], files[i], load_always))
			return false;
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Lädt die "override" lst-files aus den systemweiten und persönlichen verzeichnissen
 *
 *  @return @p true bei Erfolg, @p false bei Fehler.
 *
 *  @author FloSoft
 */
bool Loader::LoadLsts(unsigned int dir)
{
	// systemweite lsts laden
	unsigned int files_count = 0;
	unsigned int *files = NULL;

	if(GetFilePath(FILE_PATHS[dir]) == GetFilePath(FILE_PATHS[dir+3]))
	{
		files_count = 1;
		files = new unsigned int[1];
		files[0] = dir;
	}
	else
	{
		files_count = 2;
		files = new unsigned int[2];
		files[0] = dir;
		files[1] = dir + 3;
	}

	if(!LoadFilesFromArray(files_count, files) )
	{
		delete[] files;
		return false;
	}

	delete[] files;
	return true;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Lädt alle Sounds.
 *
 *  @return liefert true bei Erfolg, false bei Fehler
 *
 *  @author FloSoft
 */
bool Loader::LoadSounds(void)
{
	// ist die konvertierte sound.lst vorhanden?
	if(!FileExists(FILE_PATHS[55]))
	{
		// nein, dann konvertieren

		std::stringstream cmdss;
		cmdss << GetFilePath(FILE_PATHS[57]); // pfad zum sound-converter hinzufügen

		// name anhängen
#ifdef _WIN32
		cmdss << "\\sound-convert.exe";
#else
		cmdss << "/sound-convert";
#endif
		
		// parameter anhängen
		cmdss << " -s \"";
		cmdss << GetFilePath(FILE_PATHS[56]); // script
		cmdss << "\" -f \"";
		cmdss << GetFilePath(FILE_PATHS[49]); // quelle
		cmdss << "\" -t \"";
		cmdss << GetFilePath(FILE_PATHS[55]); // ziel
		cmdss << "\"";

		std::string cmd = cmdss.str();
#ifdef _WIN32
		std::replace(cmd.begin(), cmd.end(), '/', '\\'); // Slash in Backslash verwandeln, sonst will "system" unter win nicht
#endif // _WIN32

		if(system(cmd.c_str()) == -1)
			return false;

		// die konvertierte muss nicht extra geladen werden, da sie im override-ordner landet
	}

	// ggf original laden, hier das overriding benutzen wär ladezeitverschwendung
	if(!FileExists(FILE_PATHS[55]))
	{
		// existiert nicht 
		if(!LoadFile(FILE_PATHS[49]))
			return false;
	}

	std::list<std::string> liste;
	ListDir(GetFilePath(FILE_PATHS[50]), false, NULL, NULL, &liste);

	unsigned int i = 0;
	sng_lst.alloc(unsigned(liste.size()));
	for(std::list<std::string>::iterator it = liste.begin(); it!=liste.end(); ++it)
	{
		libsiedler2::ArchivInfo sng;

		LOG.lprintf("lade \"%s\": ", it->c_str());
		if(libsiedler2::loader::LoadSND(it->c_str(), &sng) != 0 )
		{
			LOG.lprintf("fehlgeschlagen\n");
			return false;
		}
		LOG.lprintf("fertig\n");

		sng_lst.setC(i++, sng.get(0));
	}

	// Siedler I MIDI-Musik
	//sng_lst.pushC(sound_lst.get(0));

	return true;
}

///////////////////////////////////////////////////////////////////////////////
/** 
 *  sortiert einen string nach Startzahl, Namen oder Länge (in dieser Reihenfolge).
 *  Wird für das Sortieren der Dateien benutzt.
 *
 *  @author FloSoft
 */
bool Loader::SortFilesHelper(const std::string& lhs, const std::string& rhs)
{
	int a, b;
	
	std::string lf = lhs.substr(lhs.find_last_of('/')+1);
	std::string rf = rhs.substr(rhs.find_last_of('/')+1);

	std::stringstream aa;
	aa << lf;
	std::stringstream bb;
	bb << rf;

	if( !(aa >> a) || !(bb >> b) )
	{
		for( std::string::const_iterator lit = lhs.begin(), rit = rhs.begin(); lit != lhs.end() && rit != rhs.end(); ++lit, ++rit )
			if( tolower( *lit ) < tolower( *rit ) )
				return true;
			else if( tolower( *lit ) > tolower( *rit ) )
				return false;
		if( lhs.size() < rhs.size() )
			return true;
	}
	else
	{
		if(a < b)
			return true;
	}

	return false;
}

///////////////////////////////////////////////////////////////////////////////
/** 
 *  zerlegt einen String in Einzelteile
 *  Wird für das richtige Laden der Dateien benutzt.
 *
 *  @author FloSoft
 */
std::vector<std::string> Loader::ExplodeString(std::string const &line, const char delim, const unsigned int max) 
{
	std::istringstream in(line);
	std::vector<std::string> result;
	std::string token;

	unsigned int len = 0;
	while(std::getline(in, token, delim) && result.size() < max-1) {
		len += token.size() + 1;
		result.push_back(token);
	}

	if(len < in.str().length())
		result.push_back(in.str().substr(len));

	return result;
}


///////////////////////////////////////////////////////////////////////////////
/**
 *  Lädt die Settings.
 *
 *  @return @p true bei Erfolg, @p false bei Fehler.
 *
 *  @author FloSoft
 */
bool Loader::LoadSettings()
{
	const unsigned int files_count = 1;

	const unsigned int files[files_count] = {
		0	// config.ini
	};

	if(!LoadFilesFromArray(files_count, files))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Speichert die Settings.
 *
 *  @return @p true bei Erfolg, @p false bei Fehler.
 *
 *  @author FloSoft
 */
bool Loader::SaveSettings()
{
	std::string file = GetFilePath(FILE_PATHS[0]);

	LOG.lprintf("schreibe \"%s\": ", file.c_str());
	fflush(stdout);

	if(libsiedler2::Write(file.c_str(), &files.find("config")->second) != 0)
		return false;

#ifndef _WIN32
	chmod(file.c_str(), S_IRUSR|S_IWUSR);
#endif // !_WIN32

	LOG.lprintf("fertig\n");

	return true;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Lädt die Spieldateien.
 *
 *  @param[in] gfxset  Das GFX-Set
 *  @param[in] nations Array der zu ladenden Nationen.
 *
 *  @return @p true bei Erfolg, @p false bei Fehler.
 *
 *  @author OLiver
 */
bool Loader::LoadFilesAtGame(unsigned char gfxset, bool *nations)
{
	assert(gfxset <= 2);

	const unsigned int files_count = 4 + 5 + 6 + 4 + 1 + 1;

	unsigned int files[files_count] = { 
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, // ?afr_z.lst, ?jap_z.lst, ?rom_z.lst, ?vik_z.lst
		26, 44, 45, 86, 92,                             // rom_bobs.lst, carrier.bob, jobs.bob, boat.lst, boot_z.lst
		58, 59, 60, 61, 62, 63,                         // mis0bobs.lst, mis1bobs.lst, mis2bobs.lst, mis3bobs.lst, mis4bobs.lst, mis5bobs.lst
		35, 36, 37, 38,                                 // afr_icon.lst, jap_icon.lst, rom_icon.lst, vik_icon.lst
		23 + gfxset,                                    // map_?_z.lst
		20 + gfxset                                     // tex?.lbm
	};

	for(unsigned char i = 0; i < NATION_COUNT; ++i)
	{
		// ggf. Völker-Grafiken laden
		if(nations[i])
			files[i] = 27 + i + (gfxset == 2)*NATION_COUNT;
	}

	// dateien ggf nur einmal laden
	if(!LoadFilesFromArray(files_count, files, false))
	{
		lastgfx = 0xFF;
		return false;
	}

	if(!LoadLsts(96)) // lade systemweite und persönliche lst files
	{
		lastgfx = 0xFF;
		return false;
	}

	lastgfx = gfxset;

	for (unsigned int nation = 0; nation < NATION_COUNT; ++nation)
	{
		nation_gfx[nation] = &(this->files[NATION_GFXSET_Z[lastgfx][nation]]);
	}

	map_gfx = &(this->files[MAP_GFXSET_Z[lastgfx]]);
	tex_gfx = &(this->files[TEX_GFXSET[lastgfx]]);

	return true;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Lädt Dateien von Addons.
 *
 *  @param[in] id die Addon ID
 *
 *  @return @p true bei Erfolg, @p false bei Fehler.
 *
 *  @author FloSoft
 */
bool Loader::LoadFilesFromAddon(const AddonId id)
{
	std::stringstream s;
	s << GetFilePath(FILE_PATHS[96]) << "Addon_0x" << std::setw(8) << std::setfill('0') << std::hex << id << "/";

	return LoadFileOrDir(s.str(), 96, true);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  zerschneidet die Terraintexturen.
 *
 *  @return @p true bei Erfolg, @p false bei Fehler.
 *
 *  @author OLiver
 */
bool Loader::CreateTerrainTextures(void)
{
	assert(lastgfx <= 2);

	// Unanimierte Texturen
	Rect rects[16] = {
		{0,0,48,48},
		{48,0,96,48},
		{96,0,144,48},
		{144,0,192,48},

		{0,48,48,96},
		{48,48,96,96},
		{96,48,144,96},
		{144,48,192,96},

		{0,96,48,144},
		{48,96,96,144},
		{96,96,144,144},
		{144,96,192,144},

		{0,144,48,192},
		{48,144,96,192},

		{192,48,247,104},
		{192,104,247,160},
	};

	// Ränder
	Rect rec_raender[5] =
	{
		{192,176,256,192}, // Schnee
		{192,192,256,208}, // Berg
		{192,208,256,224}, // Wste
		{192,224,256,240}, // Wiese
		{192,240,256,256}  // Wasser
	};

	// Wege
	Rect rec_roads[8] =
	{
		{192,0,242,16},
		{192,16,242,32},
		{192,32,242,48},
		{192,160,242,176},

		{242,0,256,16},
		{242,16,256,32},
		{242,32,256,48},
		{242,160,256,176},
	};

	textures.clear();
	// (unanimiertes) Terrain
	for(unsigned char i = 0; i < 14; ++i)
		ExtractTexture(&textures, rects[i]);

	// Wasser und Lava
	water.clear();
	ExtractAnimatedTexture(&water, rects[14], 8, 240);

	lava.clear();
	ExtractAnimatedTexture(&lava,  rects[15], 4, 248);

	// die 5 Ränder
	borders.clear();
	for(unsigned char i = 0; i < 5; ++i)
		ExtractTexture(&borders, rec_raender[i]);

	// Wege
	roads.clear();
	roads_points.clear();
	for(unsigned char i = 0; i < 4; ++i)
	{
		ExtractTexture(&roads, rec_roads[i]);
		ExtractTexture(&roads_points, rec_roads[4+i]);
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Extrahiert eine Textur aus den Daten.
 *
 *  @author OLiver
 */
void Loader::ExtractTexture(libsiedler2::ArchivInfo *destination, Rect &rect)
{
	glArchivItem_Bitmap_Raw bitmap;
	libsiedler2::ArchivItem_Palette *palette = GetTexPaletteN(1);
	glArchivItem_Bitmap *image = GetTexImageN(0);

	unsigned short width = rect.right - rect.left;
	unsigned short height = rect.bottom - rect.top;

	unsigned char *buffer = new unsigned char[width*height];

	memset(buffer, libsiedler2::TRANSPARENT_INDEX, width*height);
	image->print(buffer, width, height, libsiedler2::FORMAT_PALETTED, palette, 0, 0, rect.left, rect.top, width, height);
	for(unsigned int x = 0; x < (unsigned int)(width*height); ++x)
	{
		if(buffer[x] == 0)
			buffer[x] = libsiedler2::TRANSPARENT_INDEX;
	}

	bitmap.create(width, height, buffer, width, height, libsiedler2::FORMAT_PALETTED, palette);
	bitmap.setPalette(palette);
	bitmap.setFormat(libsiedler2::FORMAT_PALETTED);

	delete[] buffer;

	destination->pushC(&bitmap);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Extrahiert mehrere (animierte) Texturen aus den Daten.
 *
 *  @author OLiver
 */
void Loader::ExtractAnimatedTexture(libsiedler2::ArchivInfo *destination, Rect &rect, unsigned char color_count, unsigned char start_index)
{
	glArchivItem_Bitmap_Raw bitmap;
	libsiedler2::ArchivItem_Palette *palette = GetTexPaletteN(1);
	glArchivItem_Bitmap *image = GetTexImageN(0);

	bitmap.setPalette(palette);
	bitmap.setFormat(libsiedler2::FORMAT_PALETTED);

	unsigned short width = rect.right - rect.left;
	unsigned short height = rect.bottom - rect.top;

	unsigned char *buffer = new unsigned char[width*height];

	// Mit Startindex (also irgendeiner Farbe) füllen, um transparente Pixel und damit schwarze Punke am Rand zu verhindern
	memset(buffer, start_index, width*height);

	image->print(buffer, width, height, libsiedler2::FORMAT_PALETTED, palette, 0, 0, rect.left, rect.top, width, height);

	for(unsigned char i = 0; i < color_count; ++i)
	{
		for(unsigned int x = 0; x < width; ++x)
		{
			for(unsigned int y = 0; y < height; ++y)
			{
				if(buffer[y*width + x] >= start_index && buffer[y*width + x] < start_index+color_count)
				{
					if(++buffer[y*width + x] >= start_index+color_count)
						buffer[y*width + x] = start_index;
				}
			}
		}

		bitmap.create(width, height, buffer, width, height, libsiedler2::FORMAT_PALETTED, palette);

		destination->pushC(&bitmap);
	}

	delete[] buffer;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  @brief Lädt eine Datei in ein ArchivInfo.
 *
 *  @param[in] pfad    Der Dateipfad
 *  @param[in] palette (falls benötigt) die Palette.
 *  @param[in] archiv  Das Zielarchivinfo.
 *
 *  @return @p true bei Erfolg, @p false bei Fehler.
 *
 *  @author FloSoft
 */
bool Loader::LoadArchiv(const std::string& pfad, const libsiedler2::ArchivItem_Palette *palette, libsiedler2::ArchivInfo *archiv)
{
	unsigned int ladezeit = VideoDriverWrapper::inst().GetTickCount();

	std::string file = GetFilePath(pfad);

	LOG.lprintf("lade \"%s\": ", file.c_str());
	fflush(stdout);

	if(libsiedler2::Load(file.c_str(), archiv, palette) != 0)
	{
		LOG.lprintf("fehlgeschlagen\n");
		return false;
	}

	LOG.lprintf("fertig (%ums)\n", VideoDriverWrapper::inst().GetTickCount()-ladezeit);

	return true;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  @brief 
 *
 *  @author FloSoft
 */
bool Loader::LoadFile(const std::string& pfad, const libsiedler2::ArchivItem_Palette *palette, libsiedler2::ArchivInfo *to)
{
	bool directory = false;
#ifdef _WIN32
	if(!PathFileExistsA(GetFilePath(pfad).c_str()))
	{
		LOG.lprintf("Fehler: Datei oder Verzeichnis \"%s\" existiert nicht.\n", GetFilePath(pfad).c_str());
		return false;
	}
	if ( (GetFileAttributesA(GetFilePath(pfad).c_str()) & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
		directory = true;
#else
	struct stat file_stat;
	stat(GetFilePath(pfad).c_str(), &file_stat);
	if(S_ISDIR(file_stat.st_mode))
		directory = true;
#endif

	if(!directory)
	{
		if(!LoadArchiv(pfad, palette, to))
			return false;
		return true;
	}

	LOG.lprintf("lade Verzeichnis %s\n", GetFilePath(pfad).c_str());
	std::list<std::string> lst;
	ListDir(GetFilePath(pfad) + "/*.bmp", false, NULL, NULL, &lst);
	ListDir(GetFilePath(pfad) + "/*.txt", false, NULL, NULL, &lst);
	ListDir(GetFilePath(pfad) + "/*.ger", false, NULL, NULL, &lst);
	ListDir(GetFilePath(pfad) + "/*.eng", false, NULL, NULL, &lst);
	ListDir(GetFilePath(pfad) + "/*.fon", true, NULL, NULL, &lst);
	ListDir(GetFilePath(pfad) + "/*.empty", false, NULL, NULL, &lst);

	lst.sort(SortFilesHelper);
	
	unsigned char *buffer = new unsigned char[1000*1000*4];
	for(std::list<std::string>::iterator i = lst.begin(); i != lst.end(); ++i)
	{
		// read file number, to set the index correctly
		std::string filename = i->substr(i->find_last_of('/')+1);
		std::stringstream nrs;
		int nr = -1;
		nrs << filename;
		if(! (nrs >> nr) )
			nr = -1;
			
		// Dateiname zerlegen
		std::vector<std::string> wf = ExplodeString(*i, '.');
			
		unsigned int bobtype = libsiedler2::BOBTYPE_BITMAP_RAW;
		short nx = 0;
		short ny = 0;
		unsigned char dx = 0;
		unsigned char dy = 0;
		libsiedler2::ArchivItem *item = NULL;

		// Common
		for(std::vector<std::string>::iterator it = wf.begin(); it != wf.end(); ++it)
		{
			if(*it == "rle")
				bobtype = libsiedler2::BOBTYPE_BITMAP_RLE;
			else if(*it == "player")
				bobtype = libsiedler2::BOBTYPE_BITMAP_PLAYER;
			else if(*it == "shadow")
				bobtype = libsiedler2::BOBTYPE_BITMAP_SHADOW;

			else if(it->substr(0, 2) == "nx")
				nx = atoi(it->substr(2).c_str());
			else if(it->substr(0, 2) == "ny")
				ny = atoi(it->substr(2).c_str());
			else if(it->substr(0, 2) == "dx")
				dx = atoi(it->substr(2).c_str());
			else if(it->substr(0, 2) == "dy")
				dy = atoi(it->substr(2).c_str());
		}
		
		// Placeholder
		if( wf.back() == "empty" )
		{
			LOG.lprintf("ueberspringe %s\n", i->c_str());
			to->alloc_inc(1);
			continue;
		}

		// Bitmap
		else if( wf.back() == "bmp" )
		{
			libsiedler2::ArchivInfo temp;
			if(!LoadArchiv( *i, palette, &temp ) )
				return false;

			// Nun Daten abhängig der Typen erstellen, nur erstes Element wird bei Bitmaps konvertiert
			
			glArchivItem_Bitmap *in = dynamic_cast<glArchivItem_Bitmap*>(temp.get(0));
			glArchivItem_Bitmap *out = dynamic_cast<glArchivItem_Bitmap*>(glAllocator(bobtype, 0, NULL));

			if(!out)
			{
				LOG.lprintf("unbekannter bobtype: %d\n", bobtype);
				return false;
			}
			
			out->setName(i->c_str());
			out->setNx(nx);
			out->setNy(ny);

			memset(buffer, 0, 1000*1000*4);
			in->print(buffer, 1000, 1000, libsiedler2::FORMAT_RGBA, palette);

			switch(bobtype)
			{
			case libsiedler2::BOBTYPE_BITMAP_RLE:
			case libsiedler2::BOBTYPE_BITMAP_SHADOW:
				{
					out->create(in->getWidth(), in->getHeight(), buffer, 1000, 1000, libsiedler2::FORMAT_RGBA, palette);
				} break;
			case libsiedler2::BOBTYPE_BITMAP_PLAYER:
				{
					dynamic_cast<glArchivItem_Bitmap_Player*>(out)->create(in->getWidth(), in->getHeight(), buffer, 1000, 1000, libsiedler2::FORMAT_RGBA, palette, 128);
				} break;
			}

			item = out;
		}
		
		// Palettes
		else if( (wf.back() == "bbm") || (wf.back() == "act") )
		{
			libsiedler2::ArchivInfo temp;
			if(!LoadArchiv( *i, palette, &temp ) )
				return false;
			item = temp.get(0);
		}

		// Font
		else if( wf.back() == "fon" )
		{
			glArchivItem_Font font;
			font.setName(i->c_str());
			font.setDx(dx);
			font.setDy(dy);
			
			if(!LoadFile(*i, palette, &font))
				return false;
			
			item = &font;
		}

		if(item)
		{
			// had the filename a number? then set it to the corresponding item.
			if(nr >= 0)
			{
				if(nr >= (int)to->getCount())
					to->alloc_inc(nr - to->getCount() + 1);
				to->setC(nr, item);
			}
			else
				to->pushC(item);
		}
	}
	delete[] buffer;
	
	return true;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  @brief 
 *
 *  @author FloSoft
 */
bool Loader::LoadFile(const std::string& pfad, const libsiedler2::ArchivItem_Palette *palette, bool load_always)
{
	std::string p = pfad;
	transform ( p.begin(), p.end(), p.begin(), tolower );

	size_t pp = p.find_last_of("/\\");
	if(pp != std::string::npos)
		p = p.substr(pp+1);

	std::string e = p;
	pp = p.find_first_of('.');
	if(pp != std::string::npos)
	{
		e = p.substr(pp);
		p = p.substr(0, pp);
	}

	// bereits geladen und wir wollen kein nochmaliges laden
	if(!load_always && files.find(p) != files.end() && files.find(p)->second.getCount() != 0)
		return true;

	bool override_file = false;

	libsiedler2::ArchivInfo archiv;
	libsiedler2::ArchivInfo *to = &archiv;

	if(files.find(p) == files.end() || files.find(p)->second.getCount() == 0)
	{
		// leeres Archiv in Map einfügen
		files.insert(std::make_pair(p, archiv));
		to = &files.find(p)->second;
	}
	else
		override_file = true;
		
	if(!LoadFile(pfad, palette, to))
		return false;
	
	// haben wir eine override file? dann nicht-leere items überschreiben
	if(override_file)
	{
		LOG.lprintf("Ersetze Daten der vorher geladenen Datei\n");
		to = &files.find(p)->second;

		if(e == ".bob")
			to = dynamic_cast<libsiedler2::ArchivInfo*>(to->get(0));

		if(to == NULL)
		{
			LOG.lprintf("Fehler beim Ersetzen einer BOB-Datei\n");
			return false;
		}

		if(archiv.getCount() > to->getCount())
			to->alloc_inc(archiv.getCount()-to->getCount());

		for(unsigned int i = 0; i < archiv.getCount(); ++i)
		{
			if(archiv.get(i))
			{
				if(to->get(i))
					delete to->get(i);
				LOG.lprintf("Ersetze Eintrag %d durch %s\n", i, archiv.get(i)->getName());
				to->setC(i, archiv.get(i));
			}
		}
	}

	return true;
}
