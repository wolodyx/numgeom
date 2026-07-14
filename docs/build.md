# Сборка приложения

Предполагаем, что у вас уже есть настроенная рабочая машина по инструкциям [отсюда](vm-dev.md).
А также вы склонировали проект командой `git clone https://...`.

Сборка проекта поддерживает следующие CMake-workflow:
* `ci-windows-system`
* `ci-windows-vcpkg`
* `ci-windows-sdk`
* `ci-linux-system`
* `ci-linux-vcpkg`
* `ci-linux-sdk`

Для этого следует из командой строки (bash под Linux и powershell под Windows) выполнить команду:
```
cmake --workflow --preset ci-<windows,linux>-<vcpkg,sdk,system>
```

Но предварительно следует подготовить машину.

## `ci-windows-system`
Укажите через переменные окружения расположение пакетов Qt6 и Boost:
```
$Env:Boost_DIR = "c:/local/boost_1_91_0/lib64-msvc-14.3/cmake/Boost-1.91.0"
$Env:QT_DIR = "c:/Qt/6.11.1/msvc2022_64/lib/cmake/Qt6"
$Env:Qt6_DIR = $Env:QT_DIR
```

## `ci-windows-vcpkg`, `ci-linux-vcpkg`
Склонируйте сабмодули проекта: `git submodule update --init --recursive`.
Или же склонируйте проект вместе с сабмодулями: `git clone --recursive https://...`.

## `ci-windows-sdk`
1. Удалите предыдущую версию каталога `.sdk`: `Remove-Item -Path .sdk -Recurse -Force`.
1. Загрузите архив с sdk: `aws s3 cp s3://vcpkg-cache/sdk/windows2022 sdk.zip`.
1. Распакуйте архив в корневой каталог проекта: `Expand-Archive -Path sdk.zip -DestinationPath .`.
1. Удалите архив с sdk: `Remove-Item -Path sdk.zip -Force`.

## `ci-linux-sdk`
1. Удалите предыдущую версию каталога `.sdk`: `rm -rf .sdk`.
1. Загрузите sdk командой `aws s3 cp s3://vcpkg-cache/sdk/ubuntu24.04 sdk.zip`.
1. Распакуйте архив в корневой каталог проекта: `unzip ~/sdk.zip`.
1. Удалите архив с sdk: `rm -f sdk.zip`.
