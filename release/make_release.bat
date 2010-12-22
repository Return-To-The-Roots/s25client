@ECHO OFF

SET VERSION=0.6
SET COMPILER=vs2008

ECHO Stelle sicher, dass ein vollstaendiger Release-Build erstellt wurde
PAUSE


DEL /S /Q s25rttr_%VERSION%
MKDIR s25rttr_%VERSION%
CD s25rttr_%VERSION%

COPY /Y ..\bin\rttr.bat start.bat
COPY /Y ..\..\win32\%COMPILER%\release\mygettext.dll .
COPY /Y ..\..\win32\%COMPILER%\release\libsiedler2.dll .
COPY /Y ..\..\win32\%COMPILER%\release\s25client.exe .
COPY /Y ..\..\RTTR\texte\readme.txt .
COPY /Y ..\..\RTTR\texte\keyboardlayout.txt .

COPY /Y "F:\programmierung\VS 2005\bin\SDL.dll" .
COPY /Y "F:\programmierung\VS 2005\bin\SDL_mixer.dll" .
COPY /Y "F:\programmierung\VS 2005\bin\libbz2.dll" .
COPY /Y "F:\programmierung\VS 2008\bin\msvcr90.dll" .
COPY /Y "F:\programmierung\VS 2008\bin\msvcp90.dll" .
COPY /Y "F:\programmierung\VS 2008\bin\msvcrt.dll" .

DEL /S /Q RTTR
"F:\programmierung\VS 2008\bin\svn" --force export  ..\..\RTTR RTTR
DEL RTTR\REPLAYS\*.rpl
COPY /Y ..\..\RTTR\sound.lst RTTR\sound.lst
DEL RTTR\settings.bin
DEL RTTR\languages\rttr.pot

DEL RTTR\languages\de.po
COPY /Y ..\..\RTTR\languages\de.mo RTTR\languages\de.mo

DEL RTTR\languages\es.po
COPY /Y ..\..\RTTR\languages\es.mo RTTR\languages\es.mo

DEL RTTR\languages\fi.po
COPY /Y ..\..\RTTR\languages\fi.mo RTTR\languages\fi.mo

DEL RTTR\languages\fr.po
COPY /Y ..\..\RTTR\languages\fr.mo RTTR\languages\fr.mo

DEL RTTR\languages\hu.po
COPY /Y ..\..\RTTR\languages\hu.mo RTTR\languages\hu.mo

DEL RTTR\languages\sv.po
COPY /Y ..\..\RTTR\languages\sv.mo RTTR\languages\sv.mo

DEL RTTR\languages\ru.po
COPY /Y ..\..\RTTR\languages\ru.mo RTTR\languages\ru.mo

DEL RTTR\languages\nl.po
COPY /Y ..\..\RTTR\languages\nl.mo RTTR\languages\nl.mo

DEL RTTR\languages\et.po
COPY /Y ..\..\RTTR\languages\et.mo RTTR\languages\et.mo

DEL RTTR\languages\en_GB.po
COPY /Y ..\..\RTTR\languages\en_GB.mo RTTR\languages\en_GB.mo

DEL RTTR\languages\sk.po
COPY /Y ..\..\RTTR\languages\sk.mo RTTR\languages\sk.mo

DEL RTTR\languages\nds.po
COPY /Y ..\..\RTTR\languages\nds.mo RTTR\languages\nds.mo

DEL RTTR\languages\tr.po
COPY /Y ..\..\RTTR\languages\tr.mo RTTR\languages\tr.mo

DEL RTTR\languages\pl.po
COPY /Y ..\..\RTTR\languages\pl.mo RTTR\languages\pl.mo

DEL RTTR\languages\cs.po
COPY /Y ..\..\RTTR\languages\cs.mo RTTR\languages\cs.mo

DEL RTTR\languages\he.po
COPY /Y ..\..\RTTR\languages\he.mo RTTR\languages\he.mo

DEL RTTR\languages\it.po
COPY /Y ..\..\RTTR\languages\it.mo RTTR\languages\it.mo

DEL RTTR\languages\nb.po
COPY /Y ..\..\RTTR\languages\nb.mo RTTR\languages\nb.mo

DEL RTTR\languages\sl.po
COPY /Y ..\..\RTTR\languages\sl.mo RTTR\languages\sl.mo

MKDIR 
driver\audio
COPY /Y ..\..\driver\audio\*.dll driver\audio
DEL driver\audio\dbg_*.dll

MKDIR driver\video
COPY /Y ..\..\driver\video\*.dll driver\video
DEL driver\video\dbg_*.dll

ECHO > "put your S2-Installation here"

COPY /Y ..\..\win32\%COMPILER%\release\libsiedler2.dll RTTR
COPY /Y ..\..\win32\%COMPILER%\release\sound-convert.exe RTTR
COPY /Y "F:\programmierung\VS 2008\bin\libsamplerate.dll" RTTR
COPY /Y "F:\programmierung\VS 2008\bin\libsndfile.dll" RTTR
COPY /Y "F:\programmierung\VS 2008\bin\msvcr90.dll" RTTR
COPY /Y "F:\programmierung\VS 2008\bin\msvcp90.dll" RTTR
COPY /Y "F:\programmierung\VS 2008\bin\msvcrt.dll" RTTR

DEL *.bak

CD ..
DEL s25rttr_%VERSION%.zip
ZIP -r -9 s25rttr_%VERSION%.zip s25rttr_%VERSION% -x .svn

ECHO Release Version %VERSION% erstellt

PAUSE
