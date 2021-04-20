// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "iwLobbyConnect.h"
#include "Loader.h"
#include "RTTR_Version.h"
#include "Settings.h"
#include "WindowManager.h"
#include "controls/ctrlButton.h"
#include "controls/ctrlEdit.h"
#include "controls/ctrlOptionGroup.h"
#include "controls/ctrlText.h"
#include "desktops/dskLobby.h"
#include "iwMsgbox.h"
#include "ogl/FontStyle.h"
#include "gameData/const_gui_ids.h"
#include "liblobby/LobbyClient.h"
#include "s25util/StringConversion.h"
#include "s25util/md5.hpp"

namespace {
enum
{
    ID_txtUser,
    ID_edtUser,
    ID_txtPw,
    ID_edtPw,
    ID_txtSavePw,
    ID_optSavePw,
    ID_txtProtocol,
    ID_optProtocol,
    ID_btConnect,
    ID_btRegister,
    ID_txtStatus
};
constexpr auto md5HashLen = 32;
bool isStoredPasswordHash(const std::string& pw)
{
    return pw.size() == md5HashLen + 4 && pw.substr(0, 4) == "md5:";
}
bool isStoredPasswordHash(const std::string& storedPw, const std::string& pwToCheck)
{
    return isStoredPasswordHash(storedPw) && pwToCheck.size() == md5HashLen && storedPw.substr(4) == pwToCheck;
}
} // namespace

iwLobbyConnect::iwLobbyConnect()
    : IngameWindow(CGI_LOBBYCONNECT, IngameWindow::posLastOrCenter, Extent(500, 260), _("Connecting to Lobby"),
                   LOADER.GetImageN("resource", 41))
{
    AddText(ID_txtUser, DrawPoint(20, 40), _("Username:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    ctrlEdit* user = AddEdit(ID_edtUser, DrawPoint(260, 40), Extent(220, 22), TextureColor::Green2, NormalFont, 15);
    user->SetFocus();
    user->SetText(SETTINGS.lobby.name); //-V807

    AddText(ID_txtPw, DrawPoint(20, 70), _("Password:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    ctrlEdit* pass = AddEdit(ID_edtPw, DrawPoint(260, 70), Extent(220, 22), TextureColor::Green2, NormalFont, 0, true);
    pass->SetText(isStoredPasswordHash(SETTINGS.lobby.password) ? SETTINGS.lobby.password.substr(4) :
                                                                  SETTINGS.lobby.password);

    AddText(ID_txtSavePw, DrawPoint(20, 100), _("Save Password?"), COLOR_YELLOW, FontStyle{}, NormalFont);

    Extent btSize = Extent(105, 22);
    ctrlOptionGroup* savepassword = AddOptionGroup(ID_optSavePw, GroupSelectType::Check);
    savepassword->AddTextButton(0, DrawPoint(260, 100), btSize, TextureColor::Green2, _("No"), NormalFont);
    savepassword->AddTextButton(1, DrawPoint(375, 100), btSize, TextureColor::Green2, _("Yes"), NormalFont);
    savepassword->SetSelection((SETTINGS.lobby.save_password ? 1 : 0));

    AddText(ID_txtProtocol, DrawPoint(20, 130), _("Use IPv6:"), COLOR_YELLOW, FontStyle{}, NormalFont);

    ctrlOptionGroup* ipv6 = AddOptionGroup(ID_optProtocol, GroupSelectType::Check);
    ipv6->AddTextButton(0, DrawPoint(260, 130), btSize, TextureColor::Green2, _("IPv4"), NormalFont);
    ipv6->AddTextButton(1, DrawPoint(375, 130), btSize, TextureColor::Green2, _("IPv6"), NormalFont);
    ipv6->SetSelection((SETTINGS.server.ipv6 ? 1 : 0));

    AddText(ID_txtStatus, DrawPoint(250, 165), "", COLOR_RED, FontStyle::CENTER, NormalFont);

    btSize = Extent(220, 22);
    AddTextButton(ID_btConnect, DrawPoint(20, 190), btSize, TextureColor::Red1, _("Connect"), NormalFont);
    AddTextButton(ID_btRegister, DrawPoint(260, 190), btSize, TextureColor::Green2, _("Register"), NormalFont);

    LOBBYCLIENT.SetProgramVersion(RTTR_Version::GetReadableVersion());
    LOBBYCLIENT.AddListener(this);
}

iwLobbyConnect::~iwLobbyConnect()
{
    try
    {
        LOBBYCLIENT.RemoveListener(this);
        if(!LOBBYCLIENT.IsLoggedIn())
            LOBBYCLIENT.Stop();
    } catch(...)
    {
        // Ignored
    }
}

/**
 *  speichert die eingegebenen Benutzerdaten in die Settings
 */
void iwLobbyConnect::ReadFromEditAndSaveLobbyData(std::string& user, std::string& pass)
{
    // Dann Form abrufen
    user = GetCtrl<ctrlEdit>(ID_edtUser)->GetText();
    pass = GetCtrl<ctrlEdit>(ID_edtPw)->GetText();

    // Potential false positive: User uses new password which is equal to the hash of the old one. HIGHLY unlikely, so
    // ignore
    if(!isStoredPasswordHash(SETTINGS.lobby.password, pass))
        pass = s25util::md5(pass).toString();

    // Name speichern
    SETTINGS.lobby.name = user; //-V807

    // Ist Passwort speichern an?
    if(SETTINGS.lobby.save_password)
        SETTINGS.lobby.password = "md5:" + pass;
    else
        SETTINGS.lobby.password.clear();
}

void iwLobbyConnect::Msg_EditChange(const unsigned /*ctrl_id*/)
{
    // Statustext resetten
    SetText("", COLOR_RED, true);
}

void iwLobbyConnect::Msg_EditEnter(const unsigned ctrl_id)
{
    auto* user = GetCtrl<ctrlEdit>(ID_edtUser);
    auto* pass = GetCtrl<ctrlEdit>(ID_edtPw);
    switch(ctrl_id)
    {
        case ID_edtUser:
            user->SetFocus(false);
            pass->SetFocus(true);
            break;
        case ID_edtPw: Msg_ButtonClick(ID_btConnect); break;
    }
}

void iwLobbyConnect::Msg_ButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case ID_btConnect: // Verbinden
        {
            // Text auf "Verbinde mit Host..." setzen und Button deaktivieren
            SetText(_("Connecting with Host..."), COLOR_RED, false);

            // Form abrufen und ggf in settings speichern
            std::string user, pass;
            ReadFromEditAndSaveLobbyData(user, pass);

            // Einloggen
            if(!LOBBYCLIENT.Login(LOADER.GetTextN("client", 0),
                                  s25util::fromStringClassic<unsigned>(LOADER.GetTextN("client", 1)), user, pass,
                                  SETTINGS.server.ipv6))
            {
                SetText(_("Connection failed!"), COLOR_RED, true);
                break;
            }
        }
        break;
        case ID_btRegister: // Registrieren
        {
            WINDOWMANAGER.Show(std::make_unique<iwMsgbox>(
              _("Error"),
              _("To register, you have to create a valid board account at http://forum.siedler25.org at the moment.\n"),
              this, MsgboxButton::Ok, MsgboxIcon::ExclamationRed, 0));
        }
        break;
    }
}

void iwLobbyConnect::Msg_OptionGroupChange(const unsigned ctrl_id, const unsigned selection)
{
    switch(ctrl_id)
    {
        case ID_optSavePw: // Passwort speichern Ja/Nein
        {
            SETTINGS.lobby.save_password = (selection == 1);
        }
        break;
        case ID_optProtocol: // IPv6 Ja/Nein
        {
            SETTINGS.server.ipv6 = (selection == 1);
        }
        break;
    }
}

bool iwLobbyConnect::Msg_KeyDown(const KeyEvent& ev)
{
    if(ev.kt != KeyType::Tab)
        return false;
    auto* user = GetCtrl<ctrlEdit>(ID_edtUser);
    auto* pass = GetCtrl<ctrlEdit>(ID_edtPw);
    if(user->HasFocus())
    {
        user->SetFocus(false);
        pass->SetFocus();
    } else if(pass->HasFocus())
    {
        pass->SetFocus(false);
        user->SetFocus();
    } else
        return false;
    return true;
}

/**
 *  Setzt den Text und Schriftfarbe vom Textfeld und den Status des
 *  Buttons.
 */
void iwLobbyConnect::SetText(const std::string& text, unsigned color, bool button)
{
    auto* t = GetCtrl<ctrlText>(ID_txtStatus);
    auto* b = GetCtrl<ctrlButton>(ID_btConnect);
    auto* b2 = GetCtrl<ctrlButton>(ID_btRegister);

    // Text setzen
    t->SetTextColor(color);
    t->SetText(text);

    // Button (de)aktivieren
    b->SetEnabled(button);
    if(b2)
        b2->SetEnabled(button);
}

/**
 *  Wir wurden eingeloggt
 */
void iwLobbyConnect::LC_LoggedIn(const std::string& /*email*/)
{
    GetCtrl<ctrlButton>(ID_btRegister)->SetEnabled(false);

    WINDOWMANAGER.Switch(std::make_unique<dskLobby>());
}

/**
 *  Status: Warten auf Antwort.
 */
void iwLobbyConnect::LC_Status_Waiting()
{
    SetText(_("Waiting for Reply..."), COLOR_YELLOW, false);
}

/**
 *  Status: Benutzerdefinierter Fehler (inkl Conn-Reset u.ä)
 */
void iwLobbyConnect::LC_Status_Error(const std::string& error)
{
    SetText(error, COLOR_RED, true);
}
