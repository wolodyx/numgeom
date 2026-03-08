# Сборка приложения

Склонируйте проект вместе с сабмодулями:
```
git clone --recursive https://github.com/wolodyx/numgeom.git
```

Если склонировали без сабмодулей, то загрузите их командой:
```
git submodule update --init --recursive
```

Из корня проекта в зависимости от типа ОС запустите одну из команд:
```
./build/build-linux.sh
./build/build-windows.sh
```

Сборка проекта на рабочей машине состоит из следующих шагов:
* *одноразовая* настройка рабочего окружения командой `./build/setup-linux.sh` *с успешным завершением*;
* сборка проекта командой `./build/build-linux`.

Сборка проекта в контейнере docker состоит из следующих шагов:
* формирование образа docker;
* сборка проекта в контейнере docker.

```
docker build --no-cache -t numgeom-app:latest .
docker run -it --rm numgeom-app:latest /bin/bash
```

