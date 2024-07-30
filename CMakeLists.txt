cmake_minimum_required(VERSION 3.11)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
set(CONAN_DISABLE_CHECK_COMPILER True CACHE BOOL "" FORCE )
conan_basic_setup(TARGETS)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(game_server
	src/main.cpp
	src/logger.h
	src/http_server.cpp
	src/http_server.h
	src/sdk.h
	src/model.h
	src/model.cpp
	src/map.h
	src/map.cpp
	src/player.h
	src/player.cpp
	src/tagged.h
	src/boost_json.cpp
	src/json_loader.h
	src/json_loader.cpp
	src/request_handler.cpp
	src/request_handler.h
	src/api_handler.h
	src/api_handler.cpp
	src/uri_handler.h
	src/response_maker.h
	src/ticker.h
)
target_include_directories(game_server PRIVATE CONAN_PKG::boost)
target_link_libraries(game_server PRIVATE CONAN_PKG::boost) 
target_link_libraries(game_server PRIVATE Threads::Threads)
set_target_properties(game_server PROPERTIES test-data test-data)

# curl -H 'Content-Type: application/json' -d '{"userName": "Scooby Doo", "mapId": "map1"}' -X POST http://localhost:8080/api/v1/game/join