@echo off
rem Kubit FPS servisi kurulumu (bir kere, YONETICI olarak): Intel PresentMon 2.6.0'i GitHub'dan indirir (GameTechDev/PresentMon, ~1 MB)
rem ve SYSTEM olarak her acilista calisan "KubitFPS" gorevini kurar. Oyunun FPS'i Kubit > Bilgisayar > Canli monitor'de gorunur.
rem Kaldirmak icin: schtasks /delete /tn KubitFPS /f  ve  C:\ProgramData\KubitPC klasorunu sil.
net session >nul 2>&1 || (echo Bunu YONETICI olarak calistir: sag tikla ^> Yonetici olarak calistir & pause & exit /b 1)
set "H=C:\ProgramData\KubitPC"
if not exist "%H%" mkdir "%H%"
rem SYSTEM bu klasordeki programi calistiracak: normal kullanicilar yalniz okuyabilsin, degistiremesin
icacls "%H%" /inheritance:r /grant:r "*S-1-5-18:(OI)(CI)F" "*S-1-5-32-544:(OI)(CI)F" "*S-1-5-32-545:(OI)(CI)RX" >nul
copy /y "%~dp0fps_servis.ps1" "%H%\fps_servis.ps1" >nul
if not exist "%H%\PresentMon.exe" (
  echo PresentMon indiriliyor...
  powershell -NoProfile -Command "Invoke-WebRequest -UseBasicParsing 'https://github.com/GameTechDev/PresentMon/releases/download/v2.6.0/PresentMon-2.6.0-x64.exe' -OutFile '%H%\PresentMon.exe'"
)
if not exist "%H%\PresentMon.exe" (echo PresentMon indirilemedi. & pause & exit /b 1)
schtasks /create /tn "KubitFPS" /tr "powershell -NoProfile -ExecutionPolicy Bypass -WindowStyle Hidden -File \"%H%\fps_servis.ps1\"" /sc onstart /ru SYSTEM /rl HIGHEST /f >nul
schtasks /run /tn "KubitFPS" >nul
echo.
echo Tamam: FPS servisi calisiyor. Bir oyun ac, Kubit'te Bilgisayar ^> Canli monitor'e bak.
pause
