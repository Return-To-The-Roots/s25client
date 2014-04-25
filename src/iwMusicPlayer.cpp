// $Id: iwMusicPlayer.cpp 9357 2014-04-25 15:35:25Z FloSoft $
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
#include "iwMusicPlayer.h"

#include "Loader.h"
#include "ctrlButton.h"
#include "ctrlEdit.h"
#include "ctrlList.h"
#include "ctrlDeepening.h"
#include "ctrlComboBox.h"
#include "WindowManager.h"
#include "iwMsgbox.h"
#include "MusicPlayer.h"
#include "files.h"
#include "ListDir.h"
#include "Settings.h"


iwMusicPlayer::InputWindow::InputWindow(iwMusicPlayer* parent, const unsigned win_id, const std::string& title)
    : IngameWindow(CGI_INPUTWINDOW, (unsigned short) - 2, (unsigned short) - 2,
                   300, 100, title, LOADER.GetImageN("resource", 41), true), parent(parent), win_id(win_id)
{
    AddEdit(0, 20, 30, GetWidth() - 40, 22, TC_GREEN2, NormalFont);
    AddTextButton(1, 20, 60, 100, 22, TC_GREEN1, _("OK"), NormalFont);
    AddTextButton(2, 130, 60, 100, 22, TC_RED1, _("Abort"), NormalFont);
}


void iwMusicPlayer::InputWindow::Msg_ButtonClick(const unsigned int ctrl_id)
{
    if(ctrl_id == 1)
        parent->Msg_Input(win_id, GetCtrl<ctrlEdit>(0)->GetText());

    Close();
}

void iwMusicPlayer::InputWindow::Msg_EditEnter(const unsigned int ctrl_id)
{
    static_cast<iwMusicPlayer*>(parent)->Msg_Input(win_id, GetCtrl<ctrlEdit>(0)->GetText());
    Close();
}


iwMusicPlayer::iwMusicPlayer()
    : IngameWindow(CGI_MUSICPLAYER, (unsigned short) - 1, (unsigned short) - 1, 430, 330, _("Music player"),
                   LOADER.GetImageN("resource", 41)), changed(false)
{

    AddList(0, 20, 30, 330, 200, TC_GREEN1, NormalFont);
    AddText(1, 20, 240, _("Playlist:"), COLOR_YELLOW, 0, NormalFont);
    AddComboBox(2, 20, 260, 330, 22, TC_GREEN1, NormalFont, 200);

    // Playlistbuttons
    const unsigned short button_distance = 10;
    const unsigned short button_width = (330 - button_distance) / 2;
    ctrlButton* b1 = AddTextButton(3, 20, 290, button_width, 22, TC_GREEN2, _("Add"), NormalFont);
    AddTextButton(4, b1->GetX(false) + button_width + button_distance, 290, button_width, 22, TC_GREEN2, _("Remove"), NormalFont);
    //AddTextButton(5,b1->GetX(false),320,button_width,22,TC_GREEN2,_("Save"),NormalFont);
    //AddTextButton(6,b2->GetX(false),320,button_width,22,TC_GREEN2,_("Load"),NormalFont);

    // Buttons für die Musikstücke
    AddImageButton(7, 370, 30, 40, 40, TC_GREY, LOADER.GetImageN("io", 138), _("Add track"));
    AddImageButton(8, 370, 80, 40, 40, TC_GREY, LOADER.GetImageN("io_new", 2), _("Add directory of tracks"));
    AddImageButton(9, 370, 130, 40, 40, TC_RED1, LOADER.GetImageN("io", 220), _("Remove track"));
    AddImageButton(10, 370, 180, 40, 15, TC_GREY, LOADER.GetImageN("io", 33), _("Upwards"));
    AddImageButton(11, 370, 195, 40, 15, TC_GREY, LOADER.GetImageN("io", 34), _("Downwards"));
    AddDeepening(12, 370, 220, 40, 20, TC_GREY, "1", NormalFont, COLOR_YELLOW);
    AddImageButton(13, 370, 240, 20, 20, TC_RED1, LOADER.GetImageN("io", 139), _("Less repeats"));
    AddImageButton(14, 390, 240, 20, 20, TC_GREY, LOADER.GetImageN("io", 138), _("More repeats"));
    AddImageButton(15, 370, 270, 40, 40, TC_GREY, LOADER.GetImageN("io", 107), _("Playback in this order")); //225

    // Mit Werten füllen
    MusicPlayer::inst().GetPlaylist().FillMusicPlayer(this);
    UpdatePlaylistCombo(SETTINGS.sound.playlist);
}

iwMusicPlayer::~iwMusicPlayer()
{
    // Playlist ggf. speichern, die ausgewählt ist, falls eine ausgewählt ist
    unsigned short selection = GetCtrl<ctrlComboBox>(2)->GetSelection();


    // Entsprechende Datei speichern
    if(selection != 0xFFFF)
    {
        Playlist pl;
        pl.ReadMusicPlayer(this);

        std::string str(GetCtrl<ctrlComboBox>(2)->GetText(selection));

        // RTTR-Playlisten dürfen nicht gelöscht werden
        if(str == "S2_Standard")
            return;

        if(!pl.SaveAs(GetFullPlaylistPath(str), true))
            // Fehler, konnte nicht gespeichert werden
            WindowManager::inst().Show(new iwMsgbox(_("Error"), _("The specified file couldn't be saved!"), this, MSB_OK, MSB_EXCLAMATIONRED));
    }


    // Entsprechenden Dateipfad speichern
    if(selection != 0xFFFF)
        SETTINGS.sound.playlist = GetCtrl<ctrlComboBox>(2)->GetText(selection);

    // Werte in Musikplayer bringen
    if(changed)
    {
        MusicPlayer::inst().GetPlaylist().ReadMusicPlayer(this);
        MusicPlayer::inst().Play();
    }
}

void iwMusicPlayer::Msg_ComboSelectItem(const unsigned ctrl_id, const unsigned short selection)
{
    // Entsprechende Datei geladen
    if(selection != 0xFFFF)
    {
        Playlist pl;
        if(pl.Load(GetFullPlaylistPath(GetCtrl<ctrlComboBox>(2)->GetText(selection))))
        {
            // Das Fenster entsprechend mit den geladenen Werten füllen
            pl.FillMusicPlayer(this);
            changed = true;
        }
        else
            // Fehler, konnte nicht geladen werden
            WindowManager::inst().Show(new iwMsgbox(_("Error"), _("The specified file couldn't be loaded!"), this, MSB_OK, MSB_EXCLAMATIONRED));
    }

}

void iwMusicPlayer::Msg_ListChooseItem(const unsigned int ctrl_id, const unsigned short selection)
{
    // Werte in Musikplayer bringen
    MusicPlayer::inst().GetPlaylist().ReadMusicPlayer(this);
    MusicPlayer::inst().GetPlaylist().SetStartSong(selection);
    MusicPlayer::inst().Play();

    // Wir haben ab jetzt quasi keine Veränderungen mehr --> damit Musik nicht neugestartet werden muss
    changed = false;
}

std::string iwMusicPlayer::GetFullPlaylistPath(const std::string& combo_str)
{
    return (GetFilePath(FILE_PATHS[90]) + combo_str + ".pll");
}

void iwMusicPlayer::Msg_ButtonClick(const unsigned int ctrl_id)
{
    switch(ctrl_id)
    {
            // Add Playlist
        case 3:
        {
            WindowManager::inst().Show(new InputWindow(this, 1, _("Specify the playlist name")));
        } break;
        // Remove Playlist
        case 4:
        {
            unsigned short selection = GetCtrl<ctrlComboBox>(2)->GetSelection();

            // Entsprechende Datei löschen
            if(selection != 0xFFFF)
            {
                std::string str(GetCtrl<ctrlComboBox>(2)->GetText(selection));

                // RTTR-Playlisten dürfen nicht gelöscht werden
                if(str == "S2_Standard")
                {
                    WindowManager::inst().Show(new iwMsgbox(_("Error"), _("You are not allowed to delete the standard playlist!"), this, MSB_OK, MSB_EXCLAMATIONRED));
                    return;
                }

                unlink(GetFullPlaylistPath(str).c_str());
                this->UpdatePlaylistCombo(SETTINGS.sound.playlist);
            }
        } break;
        // Save Playlist
        case 5:
        {

        } break;
        // Load Playlist
        case 6:
        {


        } break;
        // Add Track
        case 7:
        {
            WindowManager::inst().Show(new InputWindow(this, 0, _("Add track")));
            changed = true;
        } break;
        // Add Directory of tracks
        case 8:
        {
            WindowManager::inst().Show(new InputWindow(this, 2, _("Add directory of tracks")));
            changed = true;
        } break;
        // Remove Track
        case 9:
        {
            unsigned short selection = GetCtrl<ctrlList>(0)->GetSelection();

            if(selection != 0xFFFF)
            {
                GetCtrl<ctrlList>(0)->Remove(selection);
                changed = true;
            }


        } break;
        // Upwards
        case 10:
        {
            unsigned short selection = GetCtrl<ctrlList>(0)->GetSelection();

            if(selection > 0 && selection != 0xFFFF)
                GetCtrl<ctrlList>(0)->Swap(selection - 1, selection);

        } break;
        // Downwards
        case 11:
        {
            unsigned short selection = GetCtrl<ctrlList>(0)->GetSelection();

            if(selection < GetCtrl<ctrlList>(0)->GetLineCount() - 1 && selection != 0xFFFF)
                GetCtrl<ctrlList>(0)->Swap(selection + 1, selection);

        } break;
        // Less Repeats
        case 13:
        {
            unsigned repeats = atoi(GetCtrl<ctrlDeepening>(12)->GetText().c_str());

            if(repeats)
            {
                --repeats;
                char str[32];
                sprintf(str, "%u", repeats);
                GetCtrl<ctrlDeepening>(12)->SetText(str);
                changed = true;
            }

        } break;
        // More Repeats
        case 14:
        {
            unsigned repeats = atoi(GetCtrl<ctrlDeepening>(12)->GetText().c_str());
            ++repeats;
            char str[32];
            sprintf(str, "%u", repeats);
            GetCtrl<ctrlDeepening>(12)->SetText(str);
            changed = true;
        } break;
        // Play Order
        case 15:
        {
            GetCtrl<ctrlImageButton>(15)->SetImage(GetCtrl<ctrlImageButton>(15)->GetButtonImage() ==
                                                   LOADER.GetImageN("io", 107) ? LOADER.GetImageN("io", 225) : LOADER.GetImageN("io", 107));
            GetCtrl<ctrlImageButton>(15)->SetTooltip(GetCtrl<ctrlImageButton>(15)->GetButtonImage() ==
                    LOADER.GetImageN("io", 107) ? _("Playback in this order") : _("Random playback"));
            changed = true;
        } break;



    }
}

bool ValidateFile(const std::string& filename)
{
    FILE* file = fopen(filename.c_str(), "r");
    if(file == NULL)
        return false;
    else
    {
        fclose(file);
        return true;
    }
}


void iwMusicPlayer::Msg_Input(const unsigned int win_id, const std::string& msg)
{
    switch(win_id)
    {
            // Add Track - Window
        case 0:
        {
            bool valid = false;

            // Existiert diese Datei nicht?
            if(ValidateFile(msg))
                valid = true;
            else
            {
                // Evtl ein Siedlerstück ("sNN")?
                if(msg.length() == 3)
                {
                    if(atoi(msg.substr(1).c_str()) <= 14)
                        valid = true;
                }
            }

            // Gültiges Siedlerstück?
            if(valid)
            {
                // Hinzufügen
                GetCtrl<ctrlList>(0)->AddString(msg);
                changed = true;
            }
            else
                WindowManager::inst().Show(new iwMsgbox(_("Error"), _("The specified file couldn't be opened!"), this, MSB_OK, MSB_EXCLAMATIONRED));

        } break;
        // Add Playlist
        case 1:
        {
            bool valid = true;

            // Ungültige Namen ausschließen
            if(msg.length() == 0) valid = false;
            else if(!((msg[0] >= 'a' && msg[0] <= 'z') || (msg[0] >= 'A' && msg[0] <= 'Z'))) valid = false;

            Playlist pl;
            if(!pl.SaveAs(GetFullPlaylistPath(msg), true))
                valid = false;

            if(valid)
            {
                // Combobox updaten
                UpdatePlaylistCombo(msg);
                changed = true;
            }
            else
            {
                // Fehler, konnte nicht gespeichert werden
                WindowManager::inst().Show(new iwMsgbox(_("Error"), _("The specified file couldn't be saved!"), this, MSB_OK, MSB_EXCLAMATIONRED));
            }
        } break;
        // Add Track directory of tracks - Window
        case 2:
        {
            std::list<std::string> lst;
            ListDir(msg + "/*.ogg", false, NULL, NULL, &lst);

            for(std::list<std::string>::iterator it = lst.begin(); it != lst.end(); ++it)
                GetCtrl<ctrlList>(0)->AddString(*it);

            changed = true;

        } break;

    }
}

void iwMusicPlayer::SetSegments(const std::vector<std::string>& segments)
{
    GetCtrl<ctrlList>(0)->DeleteAllItems();

    for(unsigned i = 0; i < segments.size(); ++i)
        GetCtrl<ctrlList>(0)->AddString(segments[i]);
}
void iwMusicPlayer::SetRepeats(const unsigned repeats)
{
    char repeats_str[32];
    sprintf(repeats_str, "%u", repeats);
    GetCtrl<ctrlDeepening>(12)->SetText(repeats_str);
}

void iwMusicPlayer::SetRandomPlayback(const bool random_playback)
{
    GetCtrl<ctrlImageButton>(15)->SetImage(
        random_playback ? LOADER.GetImageN("io", 225) : LOADER.GetImageN("io", 107));
}

void iwMusicPlayer::GetSegments(std::vector<std::string>& segments) const
{
    segments.clear();
    for(unsigned i = 0; i < GetCtrl<ctrlList>(0)->GetLineCount(); ++i)
        segments.push_back(GetCtrl<ctrlList>(0)->GetItemText(i));
}

unsigned iwMusicPlayer::GetRepeats() const
{
    return atoi(GetCtrl<ctrlDeepening>(12)->GetText().c_str());
}

bool iwMusicPlayer::GetRandomPlayback() const
{
    return !(GetCtrl<ctrlImageButton>(15)->GetButtonImage() ==
             LOADER.GetImageN("io", 107));
}


/// Updatet die Playlist - Combo
void iwMusicPlayer::UpdatePlaylistCombo(const std::string& highlight_entry)
{
    GetCtrl<ctrlComboBox>(2)->DeleteAllItems();

    std::list<std::string> segments;
    ListDir(std::string(FILE_PATHS[90]) + "*.pll", false, NULL, NULL, &segments);

    unsigned i = 0;
    for(std::list<std::string>::iterator it = segments.begin(); it != segments.end(); ++it, ++i)
    {
        std::string entry = it->substr(it->find_last_of("/") + 1);
        entry.erase(entry.length() - 4, 4);
        GetCtrl<ctrlComboBox>(2)->AddString(entry);
        if(entry == highlight_entry)
            GetCtrl<ctrlComboBox>(2)->SetSelection(i);
    }
}
