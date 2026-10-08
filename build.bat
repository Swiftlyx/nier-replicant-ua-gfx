@echo off
rem Builds NierReplicantUA.dll and NierReplicantGFX.dll (MSVC Build Tools 2022, x64) into out\, with
rem copies under the other names they load as: .asi, dinput8.dll (UA), xinput9_1_0.dll (GFX).
rem   build.bat                          build
rem   build.bat test [game exe] [libtp]  build and run the offline tests
rem     game exe  default: the Steam install
rem     libtp     folder with pfx_shader and resident_shader from the game's system\graphic\libtp;
rem               default: ..\..\Work\extracted\system\graphic\libtp
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
cd /d "%~dp0"
if not exist out mkdir out

rc /nologo /fo out\nier_ua.res nier_ua.rc || exit /b 1
cl /nologo /O2 /LD /W4 /MT /Fo:out\ nier_ua.c common.c out\nier_ua.res /Fe:out\NierReplicantUA.dll /link /DEF:nier_ua.def /DLL kernel32.lib || exit /b 1
copy /y out\NierReplicantUA.dll out\NierReplicantUA.asi >nul
copy /y out\NierReplicantUA.dll out\dinput8.dll >nul

rc /nologo /fo out\nier_gfx.res nier_gfx.rc || exit /b 1
cl /nologo /O2 /LD /W4 /wd4201 /MT /Fo:out\ nier_gfx.c common.c out\nier_gfx.res /Fe:out\NierReplicantGFX.dll /link /DEF:nier_gfx.def /DLL kernel32.lib || exit /b 1
copy /y out\NierReplicantGFX.dll out\NierReplicantGFX.asi >nul
copy /y out\NierReplicantGFX.dll out\xinput9_1_0.dll >nul

cl /nologo /O2 /W4 /MT /Fo:out\ test_ua.c /Fe:out\test_ua.exe /link kernel32.lib || exit /b 1
cl /nologo /O2 /W4 /wd4201 /MT /Fo:out\ test_gfx.c /Fe:out\test_gfx.exe /link kernel32.lib d3d11.lib || exit /b 1
del /q out\*.obj out\*.exp out\*.lib out\*.res 2>nul
if /i not "%1"=="test" exit /b 0

set GAME=C:\Program Files (x86)\Steam\steamapps\common\NieR Replicant ver.1.22474487139\NieR Replicant ver.1.22474487139.exe
if not "%~2"=="" set GAME=%~2
set LIBTP=..\..\Work\extracted\system\graphic\libtp
if not "%~3"=="" set LIBTP=%~3

rem regenerate both shader headers and make sure the committed ones match
python tools\gen_shader_edits.py "%LIBTP%\pfx_shader" "%LIBTP%\resident_shader" --out out\shader_edits.h --expected out\expected || exit /b 1
fc /b shader_edits.h out\shader_edits.h >nul || (echo shader_edits.h differs from the generated one & exit /b 1)
python tools\gen_ao_shader.py --out out\ao_shader.h || exit /b 1
fc /b ao_shader.h out\ao_shader.h >nul || (echo ao_shader.h differs from the generated one & exit /b 1)

cd out
echo === NierReplicantUA
.\test_ua.exe "%GAME%" || exit /b 1
echo === NierReplicantGFX
.\test_gfx.exe "%GAME%" expected || exit /b 1
