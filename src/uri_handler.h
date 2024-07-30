#ifndef __URI_HANDLER__
#define __URI_HANDLER__

#define BOOST_BEAST_USE_STD_STRING_VIEW

#pragma once
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/json.hpp>
#include "response_maker.h"

namespace http_handler {

namespace beast = boost::beast;
namespace http = beast::http;
namespace json = boost::json;

class URI_Request {
    // using namespace std::literals;
public:
    template <typename Body, typename Allocator>
    void ParceURI(http::request<Body, http::basic_fields<Allocator>>&& req) {
        response_status_ = http::status::ok;
        method_ = req.method();
        http_version_ = req.version();
        keep_alive_ = req.keep_alive();

        std::string req_body;

        // отдельно для конкретной карты в body закидываем искомую map_id, в target оставляем только "/api/v1/maps/"
        if (req.target().starts_with("/api/v1/maps/")) {
            target_ = req.target();

            std::string map_id = target_.substr(target_.find_last_of('/') + 1);
            target_ = "/api/v1/maps/";
            body_ = {{"map_id", map_id}};
            return ;
        }

        target_ = req.target();

        req_body = req.body();
        if (!req_body.empty()) {
            body_ = json::parse(req_body);
        }

        // пытаемся считать токен авторизации. Если его нет строка пустая
        try {
            auth_token_ = req.at("authorization");
            auth_token_ = auth_token_.substr(auth_token_.find_last_of(' ') + 1);
            if (auth_token_ == "Bearer") {
                throw std::invalid_argument("");
            }
        }
        catch(...) {
            auth_token_ = "";
        }

        //  пытаемся считать content_type. Если его нет строка пустая
        try {
            content_type_ = req.at("content-type");
        }
        catch(...) {
            content_type_ = "";
        }
    }

    http::verb GetMethod() const {
        return method_;
    }

    std::string GetTarget() const {
        return target_;
    }

    std::string GetAuthToken() const {
        return auth_token_;
    }

    json::value GetBody() const  {
        return body_;
    }

    unsigned GetHttpVersion() const {
        return http_version_;
    }

    bool GetKeepAlive() const {
        return keep_alive_;
    }

    http::status GetResponseStatusCode() const {
        return response_status_;
    }

    std::string GetContentType() const {
        return content_type_;
    }

    void SetResponceStatus(http::status status) {
        response_status_ = status;
    }

private:
    http::verb method_;
    std::string target_;
    json::value body_;
    std::string auth_token_;
    std::string content_type_;

    unsigned http_version_;
    bool keep_alive_;
    http::status response_status_;
};
} // namespace http_handler

#endif