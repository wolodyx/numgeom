@echo off

if not exist .git\ (
  echo "ERROR. Run script from project root: build\build-windows.bat"
  exit /b 1
)

if exist "bld\" (
  rmdir /s /q bld\
  if exist "bld\" (
    echo "ERROR. Close any applications that might be accessing the 'bld' directory."
    exit /b 2
  )
)

mkdir bld
cd bld
cmake --preset=windows-vcpkg-release .. ^
                    || (echo "CMake configure error" & exit /b 1)
cmake --build . ^
      --config=Release ^
                    || (echo "Build error" & exit /b 1)
ctest               || (echo "Testing error" & exit /b 1)
cmake --install .   || (echo "Install error" & exit /b 1)
cpack -G "ZIP" -B . || (echo "Packing error" & exit /b 1)
cd ..

