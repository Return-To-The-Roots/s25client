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
#ifndef iwSAVE_H_INCLUDED
#define iwSAVE_H_INCLUDED

#pragma once

#include "IngameWindow.h"
#include "iwDirectIPCreate.h"
#include "network/CreateServerInfo.h"

/// Fenster fürs Speichern UND(!) Laden von Spielständen
class iwSaveLoad : public IngameWindow
{
public:
    iwSaveLoad(unsigned short add_height, const std::string& window_title);

protected:
    /// Aktualisiert die Tabelle
    void RefreshTable();

private:
    /// Speichert bzw. läd die angegebene Datei
    virtual void SaveLoad() = 0;

    void Msg_EditEnter(unsigned ctrl_id) override;
    void Msg_ButtonClick(unsigned ctrl_id) override;
    void Msg_TableSelectItem(unsigned ctrl_id, int selection) override;

    /// Callbackfunktion zum Eintragen eines Spielstandes in die Tabelle
    static void FillSaveTable(const std::string& filePath, void* param);
};

class iwSave : public iwSaveLoad
{
public:
    iwSave();

private:
    // Speichert Datei
    void SaveLoad() override;

    void Msg_ComboSelectItem(unsigned ctrl_id, int selection) override;
};

class iwLoad : public iwSaveLoad
{
    /// Informationen zum Erstellen des Servers
    const CreateServerInfo csi;

public:
    iwLoad(const CreateServerInfo& csi);

private:
    /// Handle double click on the table
    void Msg_TableChooseItem(unsigned ctrl_id, unsigned selection) override;

    // Läd Datei
    void SaveLoad() override;
};

#endif // !iwSAVE_H_INCLUDED
