// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "iwSkipGFs.h"
#include "Loader.h"
#include "controls/ctrlEdit.h"
#include "helpers/Range.h"
#include "network/GameClient.h"
#include "gameData/const_gui_ids.h"
#include "s25util/StringConversion.h"
#include "s25util/colors.h"

namespace {
enum
{
    ID_lblToGf,
    ID_edtToGf,
    ID_btToGf,
    ID_lblByGf,
    ID_edtByGf,
    ID_btByGf,
    ID_btJumpPresetStart, // Must be last, one per jump preset
};
constexpr std::array jumpPresets = {
  100,
  1000,
  5000,
  10000,
};
} // namespace

iwSkipGFs::iwSkipGFs(GameWorldView& gwv)
    : IngameWindow(CGI_SKIPGFS, IngameWindow::posLastOrCenter, Extent(300, 120), _("Skip GameFrames"),
                   LOADER.getImageN("resource", 41)),
      gwv(gwv)
{
    constexpr auto lblWidth = 135;
    const auto edtOffset = contentOffset.x + lblWidth;
    constexpr auto spacing = 5;
    constexpr Extent edtSize(100, 20);
    Extent btSize(getIwSize().x - edtOffset - edtSize.x - spacing, edtSize.y);

    DrawPoint curPos(edtOffset, 28);
    ctrlEdit* edit = addEdit(ID_edtToGf, curPos, edtSize, TextureColor::Grey, NormalFont);
    edit->setFocus();
    curPos.x -= spacing;
    addText(ID_lblToGf, curPos + DrawPoint(0, 4), _("to GameFrame:"), COLOR_YELLOW, FontStyle::RIGHT, NormalFont);
    curPos.x += edtSize.x + spacing * 2;
    addTextButton(ID_btToGf, curPos, btSize, TextureColor::Green2, "->|", NormalFont);

    curPos.x = edtOffset;
    curPos.y += edtSize.y + 5;

    addEdit(ID_edtByGf, curPos, edtSize, TextureColor::Grey, NormalFont);
    curPos.x -= spacing;
    addText(ID_lblByGf, curPos + DrawPoint(0, 4), _("by GameFrames:"), COLOR_YELLOW, FontStyle::RIGHT, NormalFont);
    curPos.x += edtSize.x + spacing * 2;
    addTextButton(ID_btByGf, curPos, btSize, TextureColor::Green2, "-->", NormalFont);

    curPos.x = contentOffset.x + spacing;
    curPos.y += edtSize.y + 5;
    const auto availWidth = getIwSize().x - spacing * 2;
    btSize.x = (availWidth - spacing * (jumpPresets.size() - 1)) / jumpPresets.size();
    for(auto i : jumpPresets)
    {
        addTextButton(ID_btJumpPresetStart + i, curPos, btSize, TextureColor::Green2, "+" + std::to_string(i),
                      NormalFont);
        curPos.x += btSize.x + spacing;
    }
}

void iwSkipGFs::skipGFs(const unsigned edtCtrlId)
{
    int targetGF = s25util::fromStringClassicDef(getCtrl<ctrlEdit>(edtCtrlId)->getText(), 0);
    if(edtCtrlId == ID_edtByGf)
        targetGF += GAMECLIENT.getGFNumber();
    GAMECLIENT.skipGF(targetGF, gwv);
}

void iwSkipGFs::msgButtonClick(const unsigned ctrlId)
{
    if(ctrlId < ID_btJumpPresetStart)
        skipGFs((ctrlId == ID_btByGf) ? ID_edtByGf : ID_edtToGf);
    else
    {
        const unsigned targetGF = GAMECLIENT.getGFNumber() + ctrlId - ID_btJumpPresetStart;
        GAMECLIENT.skipGF(targetGF, gwv);
    }
}

void iwSkipGFs::msgEditEnter(const unsigned ctrlId)
{
    skipGFs(ctrlId);
}
