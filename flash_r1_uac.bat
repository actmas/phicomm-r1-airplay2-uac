@echo off
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0flash_r1_uac.ps1" %*
if errorlevel 1 pause
