@echo off
chcp 65001 > nul
echo ======================================
echo ESP32-S3 OV7670 - Build + Flash COM5
echo ======================================
idf.py set-target esp32s3
if errorlevel 1 goto :err
idf.py build
if errorlevel 1 goto :err
idf.py -p COM5 flash monitor
goto :eof
:err
echo.
echo Co loi. Hay mo ESP-IDF PowerShell/Terminal roi chay lai file nay.
pause
