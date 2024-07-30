#include "api_handler.h"

namespace http_handler {

    API_Handler::API_Handler() {
        target_to_non_authorithed_function["/api/v1/maps"] = &http_handler::API_Handler::MapList;
        target_to_non_authorithed_function["/api/v1/maps/"] = &http_handler::API_Handler::MapData;
        target_to_non_authorithed_function["/api/v1/game/join"] = &http_handler::API_Handler::JoinGame;
        target_to_non_authorithed_function["/api/v1/game/tick"] = &http_handler::API_Handler::Tick;

        target_to_authorithed_function["/api/v1/game/players"] = &http_handler::API_Handler::Players;
        target_to_authorithed_function["/api/v1/game/state"] = &http_handler::API_Handler::State;
        target_to_authorithed_function["/api/v1/game/player/action"] = &http_handler::API_Handler::Action;
    }

    API_Handler::StringResponse API_Handler::ExecuteTarget(URI_Request &request, model::Game &game) {
        // запросы НЕ требующие авторизации
        if (target_to_non_authorithed_function.count(request.GetTarget()) != 0) {
            auto funcIter = target_to_non_authorithed_function.at(request.GetTarget());

            return (this->*funcIter)(request, game);
        }

        // запросы требующие авторизации
        if (target_to_authorithed_function.count(request.GetTarget()) != 0) {
            auto funcIter = target_to_authorithed_function.at(request.GetTarget());
            
            return ExecuteAuthorithed(request, game, funcIter);
        }

        return ErrorResponce(request, http::status::bad_request, Response::AllowData::EMPTY, "badRequest", "Bad request");
    }

    // Фунция возращает StringResponce со списком карт
    API_Handler::StringResponse API_Handler::MapList(URI_Request &request, model::Game &game) {
        Response responce_;

        pt::ptree data, children;

        for (const auto& map : game.GetMaps()) {
            children.push_back(std::make_pair("", map.PrepareName()));
        }
        data.add_child("array", children);

        // Из-за специфики ptree корректируем полученые данные убирая внешние скобки
        std::string body = model::PtreeToString(data);
        body = body.substr(body.find_first_of('['));
        body = body.substr(0, body.find_last_of(']') + 1);

        return responce_.MakeStringResponse(http::status::ok,
            body, request.GetHttpVersion(), request.GetKeepAlive(),
            Response::AllowData::EMPTY, Response::ContentType::APP_JSON);
    }

    API_Handler::StringResponse API_Handler::MapData(URI_Request &request, model::Game &game) {
        Response responce_;
        
        std::string map_id = request.GetBody().at("map_id").as_string().c_str();
        // вернуть данные по карте если она есть
        auto map = game.FindMap(model::Map::Id(map_id));
        if (map != nullptr) {
            return responce_.MakeStringResponse(http::status::ok,
                map->PrintMap(), request.GetHttpVersion(), request.GetKeepAlive(),
                Response::AllowData::EMPTY, Response::ContentType::APP_JSON);
        }
        else {
            return ErrorResponce(request, http::status::not_found, Response::AllowData::EMPTY, "mapNotFound", "Map not found");
        }
    }

    // curl -d '{"userName": "Scooby Doo", "mapId": "map1"}' -X POST http://127.0.0.1:8080/api/v1/game/join
    API_Handler::StringResponse API_Handler::JoinGame(URI_Request &request, model::Game &game) {
        if (request.GetMethod() != http::verb::post) {
            return ErrorResponce(request, http::status::method_not_allowed, Response::AllowData::POST, "invalidMethod", "Only POST method is expected");
        }
        Response responce_;

        std::string username;
        std::string map_id;
        std::string body;

        try {
            auto json_body = request.GetBody();
            username = json_body.at("userName").as_string().c_str();
            map_id = json_body.at("mapId").as_string().c_str();
        } catch (...) {
            return ErrorResponce(request, http::status::bad_request, Response::AllowData::EMPTY, "invalidArgument", "Join game request parse error");
        }

        // empty name
        if (username.empty()) {
            return ErrorResponce(request, http::status::bad_request, Response::AllowData::EMPTY, "invalidArgument", "Invalid name");
        }

        auto map = game.FindMap(model::Map::Id(map_id));
        // no such map
        if (map == nullptr) {
            return ErrorResponce(request, http::status::not_found, Response::AllowData::EMPTY, "mapNotFound", "Map not found");
        }

        pt::ptree resp_body;

        model::GameSession &session = game.GetGameSession(model::Map::Id(map_id));
        std::pair<int, std::string> player_data = game.AddPlayerToSession(session, username);
        resp_body.put("authToken", player_data.second);
        resp_body.put("playerId", player_data.first);

        body = "{\"authToken\":\"" + player_data.second + "\",\"playerId\":" + std::to_string(player_data.first) +"}";

        return responce_.MakeStringResponse(http::status::ok,
                body, request.GetHttpVersion(), request.GetKeepAlive(),
                Response::AllowData::EMPTY, Response::ContentType::APP_JSON);
    }

    // curl -d '{"timeDelta": 100}' -X POST http://127.0.0.1:8080/api/v1/game/tick
    API_Handler::StringResponse API_Handler::Tick(URI_Request &request, model::Game &game) {
        // if tickrate initialized
        if (game.GetTickrate() != 0) {
            return ErrorResponce(request, http::status::bad_request, Response::AllowData::EMPTY, "badRequest", "Invalid endpoint");
        }

        if (request.GetMethod() != http::verb::post) {
            return ErrorResponce(request, http::status::method_not_allowed, Response::AllowData::POST, "invalidMethod", "Only POST method is expected");
        }
        Response responce_;

        int new_rate = 140;
        try {
            auto json_body = request.GetBody();
            new_rate = json_body.at("timeDelta").as_int64();
        } catch (...) {
            return ErrorResponce(request, http::status::bad_request, Response::AllowData::EMPTY, "invalidArgument", "Failed to parse tick request JSON");
        }

        game.SetTickrate(new_rate);
        game.UpdateStates();

        return responce_.MakeStringResponse(http::status::ok,
                "{}", request.GetHttpVersion(), request.GetKeepAlive(),
                Response::AllowData::EMPTY, Response::ContentType::APP_JSON);
    }

    // curl -H "Authorization: Bearer d098232b9cb3c9722c605a0157699e2b" -X GET http://127.0.0.1:8080/api/v1/game/players
    API_Handler::StringResponse API_Handler::Players(URI_Request &request, model::Game &game) {
        if (request.GetMethod() != http::verb::get && request.GetMethod() != http::verb::head) {
            return ErrorResponce(request, http::status::method_not_allowed, Response::AllowData::GET, "invalidMethod", "Only HEAD or GET method are expected");
        }
        Response responce_;

        std::string body;
        std::string auth_token = request.GetAuthToken();

        model::GameSession &session = game.GetGameSessionByToken(auth_token);

        body = session.GetPlayerList();
        return responce_.MakeStringResponse(http::status::ok,
                body, request.GetHttpVersion(), request.GetKeepAlive(),
                Response::AllowData::EMPTY, Response::ContentType::APP_JSON);
    }

    // curl -H "Authorization: Bearer b26454b763b8feb2aaab0b68dfa81d10" -X GET http://127.0.0.1:8080/api/v1/game/state
    API_Handler::StringResponse API_Handler::State(URI_Request &request, model::Game &game) {
        if (request.GetMethod() != http::verb::get && request.GetMethod() != http::verb::head) {
            return ErrorResponce(request, http::status::method_not_allowed, Response::AllowData::GET, "invalidMethod", "Only HEAD or GET method are expected");
        }

        Response responce_;

        std::string body;
        std::string auth_token = request.GetAuthToken();

        model::GameSession &session = game.GetGameSessionByToken(auth_token);

        body = session.GetPlayerData();
        return responce_.MakeStringResponse(http::status::ok,
                body, request.GetHttpVersion(), request.GetKeepAlive(),
                Response::AllowData::EMPTY, Response::ContentType::APP_JSON);
    }

    // curl -H "content-type: application/json" -H "Authorization: Bearer b26454b763b8feb2aaab0b68dfa81d10" -d '{"move": "D"}' -X POST http://127.0.0.1:8080/api/v1/game/player/action
    API_Handler::StringResponse API_Handler::Action(URI_Request &request, model::Game &game) {
        if (request.GetMethod() != http::verb::post) {
            return ErrorResponce(request, http::status::method_not_allowed, Response::AllowData::POST, "invalidMethod", "Only POST method is expected");
        }

        Response responce_;

        std::string valid_directions= "RLUD";
        std::string move_dir;
        try {
            auto json_body = request.GetBody();
            move_dir = json_body.at("move").as_string().c_str();
            if (valid_directions.find(move_dir) == std::string::npos && !move_dir.empty()) {
                throw std::invalid_argument("");
            }
        } catch (...) {
            return ErrorResponce(request, http::status::bad_request, Response::AllowData::EMPTY, "invalidArgument", "Failed to parse action");
        }

        if (request.GetContentType() != "application/json") {
            return ErrorResponce(request, http::status::bad_request, Response::AllowData::EMPTY, "invalidArgument", "Invalid content type");
        }

        std::string auth_token = request.GetAuthToken();
        model::GameSession &session = game.GetGameSessionByToken(auth_token);

        session.MovePlayerWithToken(auth_token, move_dir);

        return responce_.MakeStringResponse(http::status::ok,
                "{}", request.GetHttpVersion(), request.GetKeepAlive(),
                Response::AllowData::EMPTY, Response::ContentType::APP_JSON);
    }
} // namespace http_handler