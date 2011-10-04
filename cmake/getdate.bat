@echo off

if "%1"=="javadate" date /t & goto end
if "%1"=="javatime" time /t & goto end

for /f "tokens=1-4 eol=/ DELIMS=/ " %%i in ('date /t') do set BUILD=%%l%%j%%k
echo %BUILD%

:end
