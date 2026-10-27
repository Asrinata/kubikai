@echo off
setlocal
set "BURA=%~dp0"
rem Yalniz dogrudan Bluetooth goruntuleyiciyi acilisa ekler; ag sunucusu baslatmaz.
powershell -NoProfile -Command "$s=(New-Object -ComObject WScript.Shell).CreateShortcut([Environment]::GetFolderPath('Startup')+'\Kubit Ekran.lnk'); $s.TargetPath='%BURA%Kubit-Ekran.bat'; $s.WorkingDirectory='%BURA%'; $s.WindowStyle=7; $s.Save()"
call "%BURA%Kubit-Ekran.bat"
endlocal
