# Поднимаем nuget-сервер


## Работа с сервером

Для сервера понадобятся:
* каталог `$HOME/baget`, где будут складироваться пакеты, указывается в
  `docker-compose.yml -> services -> baget -> volumes`.
* каталог `$HOME/nginx/ssl` с SSL-сертификатами, указывается в
  `docker-compose.yml -> services -> nginx -> volumes`.
* id-адрес, указывается в `./nginx/baget.conf -> server_name`.
* API-ключ, указывается в `baget.env -> ApiKey`.

Сгенерируйте самоподписанный сертификат.
На место localhost подставьте ip-адрес сервера.
Ip-адрес можно извлечь командой `ip a` на сервере.

```bash
openssl req -x509 -nodes -days 365 -newkey rsa:2048 \
  -keyout ~/nginx/ssl/private.key \
  -out ~/nginx/ssl/certificate.crt \
  -subj "/CN=localhost"
```

Понадобятся следующие команды:
* `docker compose up -d` -- поднять контейнеры сервера;
* `docker compose down` -- остановить контейнеры сервера;
* `docker logs -f <id>` -- посмотреть логи сервера.


## Работа с клиентом

Если у вас **самоподписанный** сертификат, то скопируйте файл `certificate.crt`
на клиентскую машину и обновите сертификаты:
``` bash
sudo cp ~/certificate.crt /usr/local/share/ca-certificates
sudo update-ca-certificates
```

Настройте клиент, запустив с корня проекта команды:
``` bash
rm -f ~/.config/NuGet/NuGet.Config
./build/nuget-server/setup-nuget-client.sh
```

Установите переменную окружения `VCPKG_BINARY_SOURCES`:
``` bash
export VCPKG_BINARY_SOURCES="clear;nugetconfig,/home/tim/.config/NuGet/NuGet.Config,readwrite"
```

Проверьте доступность сервера с клиента командой:
``` bash
curl https://<server-name>/v3/index.json
```
