#ifndef __API_HANDLER__
#define __API_HANDLER__

#define BOOST_BEAST_USE_STD_STRING_VIEW

#pragma once
#include <unordered_map>
#include <functional>
#include <boost/property_tree/ptree.hpp>
#include <boost/asio/io_context.hpp>
#include <variant>

#include "uri_handler.h"
#include "response_maker.h"
#include "model.h"

namespace http_handler {

std::string PrintErrorResponce(std::string code, std::string messege);

namespace net = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
namespace json = boost::json;
namespace pt = boost::property_tree;

class API_Handler
{
public:
    using StringResponse = http::response<http::string_body>;
    using APIHandlerFunctionPtr = http_handler::API_Handler::StringResponse (http_handler::API_Handler::*)(http_handler::URI_Request&, model::Game&);

    API_Handler();
    StringResponse ExecuteTarget(URI_Request &request, model::Game &game);

private:
    std::unordered_map<std::string, APIHandlerFunctionPtr> target_to_non_authorithed_function;
    std::unordered_map<std::string, APIHandlerFunctionPtr> target_to_authorithed_function;

    static StringResponse ErrorResponce(URI_Request &request, http::status status, 
                std::string_view allow, std::string code, std::string messege) {
        Response responce_;
        std::string body = PrintErrorResponce(code, messege);
        request.SetResponceStatus(status);

        return responce_.MakeStringResponse(status,
            body, request.GetHttpVersion(), request.GetKeepAlive(),
            allow, Response::ContentType::APP_JSON);
    }

    template <typename Fn>
    StringResponse ExecuteAuthorithed(URI_Request &request, model::Game &game, Fn&& action) {
        std::string auth_token = request.GetAuthToken();

        if (auth_token.empty() || auth_token.size() != 32) {
            return ErrorResponce(request, http::status::unauthorized, Response::AllowData::EMPTY, "invalidToken", "Authorization header is missing");
        }

        if (!game.HaveGameSessionWithToken(auth_token)) {
            return ErrorResponce(request, http::status::unauthorized, Response::AllowData::EMPTY, "unknownToken", "Player token has not been found");
        }

        return (this->*action)(request, game);
    }

    StringResponse MapList(URI_Request &request, model::Game &game);
    StringResponse MapData(URI_Request &request, model::Game &game);
    StringResponse JoinGame(URI_Request &request, model::Game &game);
    StringResponse Tick(URI_Request &request, model::Game &game);
    StringResponse Update(URI_Request &request, model::Game &game);

    StringResponse Players(URI_Request &request, model::Game &game);
    StringResponse State(URI_Request &request, model::Game &game);
    StringResponse Action(URI_Request &request, model::Game &game);
};
} // namespace http_handler

#endif