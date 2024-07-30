Инструкция тестировалась на Conan, установленном через `pip install conan==1.52.0`. Версия 1.51 не работала.

## Сборка под Linux

При сборке под Linux обязательно указываются флаги:
* `-s compiler.libcxx=libstdc++11`
* `-s build_type=???`

Вот пример конфигурирования для Release и Debug:
```
# mkdir -p build-release 
# cd build-release
# conan install .. --build=missing -s build_type=Release -s compiler.libcxx=libstdc++11
# cmake .. -DCMAKE_BUILD_TYPE=Release
# cd ..

# mkdir -p build-debug
# cd build-debug
# conan install .. --build=missing -s build_type=Debug -s compiler.libcxx=libstdc++11
# cmake .. -DCMAKE_BUILD_TYPE=Debug
# cd ..

```

## Запуск

В папке `build` выполнить команду
```
./bin/game_server -c ../data/config.json -w ../static/ -t 10
```
После этого можно открыть в браузере:
* http://127.0.0.1:8080/api/v1/maps для получения списка карт и
* http://127.0.0.1:8080/api/v1/map/map1 для получения подробной информации о карте `map1`
* http://127.0.0.1:8080/ для чтения статического контента (в каталоге static)

## Запуск докера

Можно собирать и запускать сервер одной командой (вернее, двумя) в докере. Делается это так:

```
docker build -t my_http_server .
docker run --rm -p 80:8080 my_http_server
```
