# numgeom

NumGeom -- это расширение OpenCASCADE в области построения расчетных сеток.

## Сборка проекта

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

