@echo off
rem Kübit PC ajanını Windows açılışına ekler ve şimdi başlatır (pencere açmadan, pythonw ile).
set "BURA=%~dp0"
powershell -NoProfile -Command "$s=(New-Object -ComObject WScript.Shell).CreateShortcut([Environment]::GetFolderPath('Startup')+'\Kubit PC.lnk'); $s.TargetPath='pyw.exe'; $s.Arguments='\"%BURA%kubit_pc.py\"'; $s.WorkingDirectory='%BURA%'; $s.Save()"
echo Yalniz ekran paylasimi icin Kubit-Ekran.bat kullan: ag/firewall ayari gerekmez.
start "" pyw.exe "%BURA%kubit_pc.py"
timeout /t 2 >nul
echo.
echo Kubit PC ajani calisiyor ve her acilista kendiliginden baslayacak.
echo Telefonda Kubit uygulamasi ^> Ayarlar ^> "Bilgisayari esle"ye bas, burada cikan pencerede Evet de.
pause
