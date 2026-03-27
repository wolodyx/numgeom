#!/bin/bash

# Имя скрипта для сообщений об ошибках.
SCRIPT_NAME=$(basename "$0")

usage() {
  cat << EOF
Использование: $SCRIPT_NAME --arch ARCH --version VERSION --path PATH [--verbose] [--help]

Обязательные опции:
  --arch ARCH        Архитектура (например, amd64, arm64)
  --version VERSION  Версия пакета
  --path INSTALL_DIR Путь к файлам

Опции:
  --verbose          Подробный вывод
  --help             Показать эту справку
EOF
    exit 0
}

# Проверка наличия GNU getopt.
if ! command -v getopt >/dev/null 2>&1; then
  echo "Ошибка: утилита getopt не найдена. Установите util-linux." >&2
  exit 1
fi

OPTIONS=$(getopt -o '' --long arch:,version:,path:,verbose,help -n "$SCRIPT_NAME" -- "$@")
if [ $? -ne 0 ]; then
  exit 1
fi

# Применяем полученный список аргументов.
eval set -- "$OPTIONS"

# Обработка опций и инициализация переменных.
ARCH=""
VERSION=""
INSTALL_DIR=""
VERBOSE=false

while true; do
  case "$1" in
    --arch)
      ARCH="$2"
      shift 2
      ;;
    --version)
      VERSION="$2"
      shift 2
      ;;
    --path)
      INSTALL_DIR="$2"
      shift 2
      ;;
    --verbose)
      VERBOSE=true
      shift
      ;;
    --help)
      usage
      ;;
    --)
      shift
      break
      ;;
    *)
      echo "Внутренняя ошибка при разборе опций" >&2
      exit 1
      ;;
  esac
done

# Проверка обязательных опций
if [ -z "$ARCH" ] || [ -z "$VERSION" ] || [ -z "$INSTALL_DIR" ]; then
  echo "Ошибка: не указаны все обязательные опции (--arch, --version, --path)." >&2
  usage
fi

# Если включён verbose, выводим значения
if [ "$VERBOSE" = true ]; then
  echo "Архитектура: $ARCH"
  echo "Версия: $VERSION"
  echo "Путь: $INSTALL_DIR"
fi

# Основная логика сценария.
mkdir -p package/opt/numgeom/{bin,lib}
cp -r ${INSTALL_DIR}/bin package/opt/numgeom
cp -r ${INSTALL_DIR}/lib package/opt/numgeom
dpkg-deb --build ./package numgeom_${VERSION}_${ARCH}.deb
