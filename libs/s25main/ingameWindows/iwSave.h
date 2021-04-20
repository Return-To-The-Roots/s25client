// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "IngameWindow.h"
#include "network/CreateServerInfo.h"
#include <boost/filesystem/path.hpp>

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
    void Msg_TableSelectItem(unsigned ctrl_id, const boost::optional<unsigned>& selection) override;

    /// Callbackfunktion zum Eintragen eines Spielstandes in die Tabelle
    static void FillSaveTable(const boost::filesystem::path& filePath, void* param);
};

class iwSave : public iwSaveLoad
{
public:
    iwSave();

private:
    // Speichert Datei
    void SaveLoad() override;

    void Msg_ComboSelectItem(unsigned ctrl_id, unsigned selection) override;
};

class iwLoad : public iwSaveLoad
{
    /// Informationen zum Erstellen des Servers
    const CreateServerInfo csi;

public:
    iwLoad(CreateServerInfo csi);

private:
    /// Handle double click on the table
    void Msg_TableChooseItem(unsigned ctrl_id, unsigned selection) override;

    // Läd Datei
    void SaveLoad() override;
};
