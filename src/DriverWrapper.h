// $Id: DriverWrapper.h 9357 2014-04-25 15:35:25Z FloSoft $
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
#ifndef DRIVERWRAPPER_H_INCLUDED
#define DRIVERWRAPPER_H_INCLUDED

#ifdef DriverType
#undef DriverType
#endif

///////////////////////////////////////////////////////////////////////////////
// DriverWrapper
class DriverWrapper
{
    public:
        enum DriverType
        {
            DT_VIDEO = 0,
            DT_AUDIO
        };

        class DriverItem
        {
            public:
                DriverItem(std::string file, std::string name) : file(file), name(name) {}
                const std::string& GetFile() { return file; }
                const std::string& GetName() { return name; }

            private:
                std::string file, name;
        };

    public:
        /// Konstruktor von @p DriverWrapper
        DriverWrapper(void);
        /// Destruktor von @p DriverWrapper
        ~DriverWrapper(void);

        /// Läd einen Treiber in die Treiber DLL, versucht, "preference" zu nehmen
        bool Load(const DriverType dt, std::string& preference);
        /// Gibt eine Treiber-Handle wieder frei
        void Unload();
        /// Gibt Adresse auf eine bestimmte Funktion zurück
        void* GetDLLFunction(const std::string& name);

        /// Läd eine Liste von verfügbaren Treibern
        static void LoadDriverList(const DriverType dt, list<DriverItem>& driver_list);

    private:
        /// Handle auf die DLL
        HINSTANCE dll;
};

#endif // DRIVERWRAPPER_H_INCLUDED
