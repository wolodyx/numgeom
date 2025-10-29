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

vcpkg.exe install gtest nlohmann-json
vcpkg.exe install boost-log
vcpkg.exe install glslang[tools] vulkan-sdk-components
vcpkg.exe install vulkan-validationlayers
vcpkg.exe install qt5-base[vulkan]
vcpkg.exe --triplet x64-windows install opencascade
REM vcpkg.exe --triplet x64-windows install boost-program-options
