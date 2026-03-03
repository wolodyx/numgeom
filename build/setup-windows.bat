@echo off

set projectDir=%cd%
if not exist "%projectDir%\.git" (
  echo "ERROR! Run this script from the project root: build\%~nx0"
  exit /b 1
)

cd %USERPROFILE%

if exist vcpkg\ (
  rmdir /s /q vcpkg
)

git clone https://github.com/microsoft/vcpkg ^
    || (echo "vcpkg repo cloning error" & exit /b 2)
cd vcpkg
call bootstrap-vcpkg.bat -disableMetrics

vcpkg.exe install           ^
    gtest                   ^
    nlohmann-json           ^
    boost-log               ^
    vulkan-sdk-components   ^
    python3                 ^
    opencascade             ^
    qt5-base[vulkan]

vcpkg.exe install winflexbison --overlay-ports=%projectDir%\build\vcpkg\ports
