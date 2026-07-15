# Подготовка машин для разработки

Два типа машин: виртуальные и физические.
Виртуальные машины подготавливаются в VirtualBox версии 7.2.6

Форматы имен виртуальных машин:
```
numgeom-[dev,run]-[ubuntu24.04.3,windows10,windows11]-[amd64]
```

Имена машин и пользователей выбираем покороче -- 3 символа.
Примеры: dev, run, max, bob, mix, box, pix, red, ben.


## Подготовка ВМ Ubuntu-24.04

Для виртуальной машины выбираем:
* не менее 4 ядер;
* не менее 10 ГБ RAM;
* 200 ГБ диска.

``` warning
Если при сборке приложения неожиданно закроется терминал, значит машине не достаточно RAM.
Добавьте оперативной памяти в настройках ВМ.
```

1. Загрузите образ ОС ubuntu-24.04.3 из [ubuntu.com](https://ubuntu.com/) и установите ее на ВМ.
1. Сохраните состояние ВМ, предварительно выключив ее.
1. Установите `Git` командой `sudo apt -y install git`.
1. Склонируйте репозиторий командой `git clone https://github.com/wolodyx/numgeom.git ~/numgeom`.
1. Запустите настройку машины для сборки проекта командой `cd ~/numgeom && sudo ./devops/setup-ubuntu24.04.sh`.
1. Установите расширение гостевой ОС (Guest Additions) и включите "Bidirectional Shared Clipboard" в настройках ВМ.
1. Установите вспомогательные утилиты: `sudo apt -y install tmux git-gui mc`.
1. Установите [Visual Studio Code](https://code.visualstudio.com/).
1. Установите [aws-cli](https://docs.aws.amazon.com/cli/latest/userguide/getting-started-install.html).
1. Установите русскую раскладку клавиатуры: `Settings -> Keyboard -> Add Input Source... -> Other -> Russian`.
1. Настройте переключение раскладки клавиатуры через клавишу CapsLock, установив `gnome-tweaks` и пройдясь через `gnome-tweaks` -> `Keyboard` -> `Additional Layout Options` -> `Switching to another layout` -> `Caps Lock`.
1. Настройте кеширование Vcpkg.
1. Задайте настройки `user.name` и `user.email` у клиента Git (`git config --global user.name "Your Name").
1. Отображение текста в utf8 в клиенте Git: `git config --global gui.encoding utf-8`.
1. Загрузите sdk командой `aws s3 cp s3://vcpkg-cache/sdk/ubuntu24.04 ~/sdk.zip`.

### Настройка кеширования Vcpkg:
1. На хост-машине создайте две папки для кеширования Vcpkg: `D:\vcpkg-cache` и `D:\vcpkg-asset-sources`.
2. Добавьте в ВМ две общие папки (Settings -> Shared Folders) со следующими параметрами:
```
Folder Path: D:\vcpkg-asset-sources
Foler Name: vcpkg-asset-sources
Mount Point: ~
Auto-mount: on
```
```
Folder Path: D:\vcpkg-cache
Foler Name: vcpkg-cache
Mount Point: ~
Auto-mount: on
```
3. Выполните следующие команды:
```
sudo usermod -aG vboxsf $(whoami)
mkdir ~/vcpkg-cache
mkdir ~/vcpkg-asset-sources
sudo mount -t vboxsf vcpkg-cache ~/vcpkg-cache
sudo mount -t vboxsf vcpkg-asset-sources ~/vcpkg-asset-sources
```
4. Проверьте, что в домашнем каталоге появились `vcpkg-cache` и `vcpkg-asset-sources`.
5. Добавьте следующие строки в `~/.bashrc`, подставив туда свои данные:
```
export AWS_ACCESS_KEY_ID=<your_access_key_id>
export AWS_SECRET_ACCESS_KEY=<your_secret_access_key>
export AWS_DEFAULT_REGION=ru-central1
export AWS_DEFAULT_BUCKET=vcpkg-cache
export AWS_ENDPOINT_URL=https://storage.yandexcloud.net
export X_VCPKG_ASSET_SOURCES="clear;x-azurl,file://$HOME/vcpkg-assets-sources,,readwrite"
export VCPKG_BINARY_SOURCES="clear;files,$HOME/vcpkg-cache,readwrite;x-aws,s3://vcpkg-cache/vcpkg-cache/,readwrite"
```
6. Переоткройте терминал.

### Инструкция для проверки:
1. Соберите проект по пресетам `ci-linux-system`, `ci-linux-sdk`, `ci-linux-vcpkg`.
1. Запустите собранное демо-приложение.
1. Установите собранные пакеты и запустите демо-приложение из системы.
1. Откройте проект в "Visual Studio Code" и запустите демо-приложение через Ctrl-F5.

### Возможные проблемы
1. Зависание при скачивании некоторых пакетов (bzip2).
  Необходимо включить VPN при первой сборке.
  При последующих сборках уже можно пропустить VPN.
1. Зависание при загрузке рабочего стола.
  Попробуйте минимизировать и максимизировать окно ВМ.
  Если это не помогает, выключите машину и включите ее заново.
  Если все равно не помогает, переключитесь из Wayland в Xorg.


## Подготовка ВМ Windows-11

Машина под Windows настраивается под сборку проекта с конфигурациями:
* `ci-windows-system` (system);
* `ci-windows-vcpkg` (vcpkg);
* `ci-windows-sdk` (sdk).

Каждая из конфигураций требует специфичных только для нее настроек, которые будут указаны в скобках как `system`, `vcpkg`, `sdk`.

1. Установите вспомогательные программы:
  * ["Google Chrome"](https://www.google.com/chrome/);
  * ["PowerShell 7"](https://github.com/PowerShell/PowerShell);
  * ["Python 3"](https://www.python.org/downloads/)
  * ["Far Manager"](https://www.farmanager.com/download.php);
  * ["Git"](https://gitforwindows.org/);
  * ["Notepad++"](https://notepad-plus-plus.org/downloads/);
  * ["Visual Studio Code"](https://code.visualstudio.com/);
  * ["aws-cli"](https://docs.aws.amazon.com/cli/latest/userguide/getting-started-install.html) (`vcpkg`, `sdk`);
1. Установите инструменты для создания инсталляторов:
  * [NSIS](https://nsis.sourceforge.io/Download);
  * [WiX](#установка-wix);
1. Настройте [разделяемые каталоги](#разделяемые-каталоги) (`vcpkg`).
1. Добавьте [следующие переменные окружения](#переменные-окружения)
1. Задайте настройки `user.name` и `user.email` у клиента Git (`git config --global user.name "Your Name").
1. Отображение текста в utf8 в клиенте Git: `git config --global gui.encoding utf-8`.
1. Загрузите sdk командой `aws s3 cp s3://vcpkg-cache/sdk/windows2022 ~/sdk.zip` (`sdk`).
1. Установите пакеты (`system`):
  * ["Vulkan SDK"](https://sdk.lunarg.com/sdk/download/1.4.350.0/windows/vulkansdk-windows-X64-1.4.350.0.exe);
  * ["Boost"](https://archives.boost.io/release/1.91.0/binaries/boost_1_91_0-msvc-14.3-64.exe);
  * ["Qt6 msvc2022_64"](https://doc.qt.io/qt-6/qt-online-installation.html);

### Установка WiX
```
winget install Microsoft.DotNet.SDK.10
dotnet tool install --global wix --version 4.0.4
wix extension add --global WixToolset.UI.wixext/4.0.4
```

### Переменные окружения
```
AWS_ACCESS_KEY_ID=<your_access_key_id>
AWS_SECRET_ACCESS_KEY=<your_secret_access_key>
AWS_DEFAULT_REGION=ru-central1
AWS_DEFAULT_BUCKET=vcpkg-cache
AWS_ENDPOINT_URL=https://storage.yandexcloud.net
X_VCPKG_ASSET_SOURCES=clear;x-azurl,file://X:/vcpkg-assets-sources,,readwrite
VCPKG_BINARY_SOURCES=clear;files,Y:/vcpkg-cache,readwrite;x-aws,s3://vcpkg-cache/vcpkg-cache/,readwrite
```

### Разделяемые каталоги
Добавьте в ВМ две общие папки (Settings -> Shared Folders) со следующими параметрами:
```
Folder Path: D:\vcpkg-asset-sources
Foler Name: vcpkg-asset-sources
Mount Point: X:
Auto-mount: on
```
```
Folder Path: D:\vcpkg-cache
Foler Name: vcpkg-cache
Mount Point: Y:
Auto-mount: on
```
Проверьте, что в сетевой папке появились `X:\vcpkg-cache` и `Y:\vcpkg-asset-sources`.

### Инструкция для проверки
1. Запустите программу `C:\VulkanSDK\1.4.350.0\Bin\vkcube.exe`.
1. Соберите проект по пресетам `ci-windows-system`, `ci-windows-sdk`, `ci-windows-vcpkg`.
1. Запустите собранные демо-приложения.
1. Установите собранные инсталляторы и запустите демо-приложение из системы.
1. Откройте проект в "Visual Studio Code" и запустите демо-приложение через Ctrl-F5.
