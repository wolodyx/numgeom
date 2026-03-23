# Поднимаем nuget-сервер

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

Поднимите контейнеры сервера:
``` bash
docker compose up -d
```

Файл **самоподписанного** сертификата `certificate.crt` скопируйте на клиентскую
машину и обновите сертификаты.
Для доверенного сертификата это не нужно делать.
``` bash
sudo cp ~/certificate.crt /usr/local/share/ca-certificates
sudo update-ca-certificates
```

Проверьте доступность сервера с клиента командой:
``` bash
curl -k https://localhost/v3/index.json
```

Остановите контейнеры сервера:
``` bash
docker compose down
```
