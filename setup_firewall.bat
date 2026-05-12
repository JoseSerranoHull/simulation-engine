@echo off
echo =====================================================
echo  Simulation Engine - Windows Firewall Setup
echo  Run this ONCE on EVERY machine before the demo.
echo  Requires Administrator rights.
echo =====================================================
echo.

netsh advfirewall firewall add rule ^
    name="SimEngine-UDP-In-Game" ^
    protocol=UDP dir=in localport=54000-54003 action=allow
if %errorlevel% neq 0 ( echo WARNING: Game port rule failed - check admin rights )

netsh advfirewall firewall add rule ^
    name="SimEngine-UDP-In-Discovery" ^
    protocol=UDP dir=in localport=54998 action=allow
if %errorlevel% neq 0 ( echo WARNING: Discovery port rule failed )

netsh advfirewall firewall add rule ^
    name="SimEngine-UDP-In-Manual" ^
    protocol=UDP dir=in localport=7000-7003 action=allow
if %errorlevel% neq 0 ( echo WARNING: Manual connect port rule failed )

echo.
echo Done. Inbound UDP allowed on ports 54000-54003, 54998, 7000-7003.
echo Run this script on EVERY lab machine before the demo.
pause
