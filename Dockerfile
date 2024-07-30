# Не просто создаём образ, но даём ему имя build
FROM gcc:11.3 as build

RUN apt update && \
    apt install -y \
      python3-pip \
      cmake \
    && \
    pip install --force-reinstall -v "conan<2"

# Запуск conan как раньше
COPY conanfile.txt /app/
RUN mkdir /app/build && cd /app/build && \
    conan install .. -s compiler.libcxx=libstdc++11

# Папка data больше не нужна
COPY ./src /app/src
COPY CMakeLists.txt /app/

RUN cd /app/build && \
    cmake .. && \
    cmake --build .

# Второй контейнер в том же докерфайле
FROM ubuntu:22.04 as run

# Создадим пользователя www
RUN groupadd -r www && useradd -r -g www www
USER www

# Скопируем приложение со сборочного контейнера в директорию /app.
# Не забываем также папку data, она пригодится.
COPY --from=build /app/build/bin/game_server /app/
COPY ./data /app/data
COPY ./static /app/static

# Запускаем игровой сервер
# , "/app/static"
ENTRYPOINT ["/app/game_server", "-c", "/app/data/config.json", "-w", "/app/static", "-t", "10"]