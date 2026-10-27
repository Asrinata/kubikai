@echo off
setlocal
set "BURA=%~dp0"
py -3.13 -c "import sys; sys.path.insert(0,r'%BURA%_deps'); import bleak" >nul 2>&1
if errorlevel 1 (
  echo Bluetooth destegi hazirlaniyor. Ilk kurulumda internet gerekir.
  py -3.13 -m pip install --target "%BURA%_deps" "bleak>=1.0,<4"
  if errorlevel 1 (
    echo Kurulum tamamlanamadi. Python 3.13 kurulu olmali.
    pause
    exit /b 1
  )
)
start "" pyw -3.13 "%BURA%kubit_ekran.py"
endlocal
