@echo off
echo Input src file: %1
echo Output hash file: %2

set "input=%1"
set "output=%2"
certutil -hashfile "%input%" SHA256 > "%output%.bak"

for /f "tokens=1" %%a in ('certutil -hashfile "%1" SHA256 ^| find /i /v "certutil" ^| find /i /v "SHA256" ^| find /i /v "hash"') do (
    set "line=%%a"
    goto :writehash
)

:writehash
setlocal enabledelayedexpansion
echo !line: =! > "%2"
endlocal
