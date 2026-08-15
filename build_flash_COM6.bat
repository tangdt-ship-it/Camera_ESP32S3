@echo off
chcp 65001 > nul
echo ======================================
echo ESP32-S3 OV7670 V1.1 - Build + Flash COM6
echo ======================================
idf.py build
if errorlevel 1 goto :err
idf.py -p COM6 flash monitor
goto :eof
:err
echo.
echo Co loi. Hay mo ESP-IDF 5.5 PowerShell/Terminal roi chay lai file nay.
pause
