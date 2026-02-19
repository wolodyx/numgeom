# Настройка рабочего окружения в Linux

## Установка man-страниц Vulkan

Скачайте архив [с man-страницами Vulkan](https://github.com/haxpor/Vulkan-Docs/releases).
Распакуйте архив в `$HOME/share/man/man3`.
Сожмите распакованные файлы командой `gzip -r .`, предварительно перейдя в этот каталог.
Добавьте переменную окружения `MANPATH=$HOME/share/man`: в конец файла `~/.bashrc` добавьте строку `export MANPATH=$HOME/share/man`.
Перегрузите терминал.
