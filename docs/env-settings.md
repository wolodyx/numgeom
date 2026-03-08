# Настройка рабочего окружения в Linux

## Установка man-страниц Vulkan

Скачайте архив [с man-страницами Vulkan](https://github.com/haxpor/Vulkan-Docs/releases).
Распакуйте архив в `$HOME/share/man/man3`.
Сожмите распакованные файлы командой `gzip -r .`, предварительно перейдя в этот каталог.
Добавьте переменную окружения `MANPATH=$HOME/share/man`: в конец файла `~/.bashrc` добавьте строку `export MANPATH=$HOME/share/man`.
Перегрузите терминал.


## Настройка рабочей среды в Ubuntu 24.04

Предполагаем, что у вас уже есть установленная `Ubuntu 24.04`.

Если создаете виртуальную машину, выберите не менее 4 ядер процессора, не менее 8Гб RAM и 100Гб дисковой памяти.

Из корня проекта запустите команды:
```
./build/setup-linux.sh
./build/build-linux.sh
```

Если при сборке проекта будет закрываться окно терминала, то увеличьте объем RAM.

Для разработки дополнительно установите следующие приложения:
* `Vim`, `Git`, `build-essential`;
* `VirtualBox Guest Extensions`;
* `Visual Studio Code`;
* [`Docker`](https://docs.docker.com/engine/install/ubuntu/);


## Настройка рабочей среды в Windows 11

* установите `Git`, `CMake`;
* установите `Visual Studio Community`;
* установите переменную окружения `VCPKG_ROOT=%USERPROFILE%/vcpkg`;
* добавьте к `PATH` значение `%USERPROFILE%/vcpkg`;
* Из корня проекта запустите команду `./build/setup-windows.sh && ./build/build-windows.sh`.
