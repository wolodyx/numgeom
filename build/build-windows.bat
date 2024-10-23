@echo off

if not exist .git\ (
  echo "ERROR. Run script from project root: build\build-windows.bat"
  exit /b 1
)

rmdir /s /q bld
mkdir bld
cd bld
cmake --preset=windows-release .. ^
                    || (echo "CMake configure error" & exit /b 1)
cmake --build . ^
      --config=Release ^
                    || (echo "Build error" & exit /b 1)
ctest               || (echo "Testing error" & exit /b 1)
cmake --install .   || (echo "Install error" & exit /b 1)
cpack -G "ZIP" -B . || (echo "Packing error" & exit /b 1)
cd ..

