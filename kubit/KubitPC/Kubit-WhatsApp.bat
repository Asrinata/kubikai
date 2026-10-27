@echo off
setlocal
set "BURA=%~dp0"
cd /d "%BURA%"
if not exist "node_modules\whatsapp-web.js\package.json" (
  set "PUPPETEER_SKIP_DOWNLOAD=1"
  call npm.cmd ci --no-audit --no-fund
  if errorlevel 1 (
    pause
    exit /b 1
  )
)
py -3.13 -c "import sys;sys.path.insert(0,r'%BURA%_deps');import bleak" >nul 2>&1
if errorlevel 1 py -3.13 -m pip install --target "%BURA%_deps" "bleak>=1.0,<4"
start "" pyw -3.13 "%BURA%whatsapp_kopru.py"
endlocal
