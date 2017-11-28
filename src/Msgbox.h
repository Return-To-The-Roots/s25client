// Copyright (c) 2005-2017 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Siedler II.5 RTTR.
//
// Siedler II.5 RTTR is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Siedler II.5 RTTR is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Siedler II.5 RTTR. If not, see <http://www.gnu.org/licenses/>.
#ifndef MSGBOX_H_INCLUDED
#define MSGBOX_H_INCLUDED

#pragma once

enum MsgboxButton
{
    MSB_OK,
    MSB_OKCANCEL,
    MSB_YESNO,
    MSB_YESNOCANCEL
};

enum MsgboxIcon
{
    MSB_QUESTIONGREEN = 72,
    MSB_EXCLAMATIONGREEN,
    MSB_QUESTIONRED,
    MSB_EXCLAMATIONRED
};

enum MsgboxResult
{
    MSR_OK = 0,
    MSR_CANCEL,
    MSR_YES,
    MSR_NO,
    MSR_NOTHING
};

#endif //! MSGBOX_H_INCLUDED
