@ECHO OFF

REM Copyright (C) 2005 - 2021 Settlers Freaks <sf-team at siedler25.org>
REM
REM SPDX-License-Identifier: GPL-2.0-or-later

IF EXIST RTTR\s25update.exe GOTO UPDATE
GOTO START

:UPDATE
RTTR\s25update.exe --dir %CD% @STABLE_PARAM@

:START
s25client.exe

IF %ERRORLEVEL% EQU 0 GOTO END
PAUSE

:END
