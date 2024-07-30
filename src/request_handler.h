#ifndef __REQUEST_HANDLER__
#define __REQUEST_HANDLER__

#define BOOST_BEAST_USE_STD_STRING_VIEW

#pragma once
#include <boost/asio/ip/tcp.hpp>
#include <boost/json.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/asio/io_context.hpp>
#include <filesystem>
#include <unordered_map>
#include <chrono>
#include <functional>
#include <sstream>

#include "http_server.h"
#include "model.h"
#include "logger.h"
#include "api_handler.h"
#include "response_maker.h"

namespace http_handler {

namespace net = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
namespace sys = boost::system;
namespace pt = boost::property_tree;
namespace fs = std::filesystem;
namespace json = boost::json;

using namespace std::chrono;
using namespace std::literals;

std::string DecodeURL(const std::string value);
fs::path StringToPath(std::string str);
std::string PrintErrorResponce(std::string code, std::string messege);
bool IsSubPath(fs::path path, fs::path base);

struct ResponseData {
    http::status status;
    std::string_view content_type;
};

template <typename Body, typename Allocator, typename Send>
class StrandAPIRequest: public std::enable_shared_from_this<StrandAPIRequest<Body, Allocator, Send>> {
public:
    StrandAPIRequest(net::strand<net::io_context::executor_type> &strand,
        http::request<Body, http::basic_fields<Allocator>>&& req,
        Send&& send, std::string &target, ResponseData &data, API_Handler &api_handler, model::Game &game):
            strand_{strand},
            req_{std::move(req)},
            send_{std::move(send)},
            target_{target},
            data_{data}, 
            api_handler_{api_handler},
            game_{game} {}

    void Execute() {
        net::dispatch(strand_, [self=this->shared_from_this()](){
            self->ExecuteApi();
        });
    }

private:
    net::strand<net::io_context::executor_type> &strand_;
    http::request<Body, http::basic_fields<Allocator>> req_;
    Send send_;
    std::string &target_;
    ResponseData &data_;
    API_Handler &api_handler_;
    model::Game &game_;

    void ExecuteApi() {
        URI_Request request;
        request.ParceURI(std::forward<decltype(req_)>(req_));

        // выполняем запрос сохраняя responce status_code в request
        auto string_responce = api_handler_.ExecuteTarget(request, game_);

        // заполняем данные для логирования
        data_.status = request.GetResponseStatusCode();
        data_.content_type = Response::ContentType::APP_JSON;
        send_(string_responce);
    }
};

class RequestHandler {
public:
    // Запрос, тело которого представлено в виде строки
    using StringRequest = http::request<http::string_body>;
    // Ответ, тело которого представлено в виде строки
    using StringResponse = http::response<http::string_body>;

    explicit RequestHandler(net::strand<net::io_context::executor_type> &strand, model::Game& game, std::string static_folder)
        : strand_{strand}, game_{game}, static_path_{StringToPath(static_folder)}, static_folder_str_(static_folder) {
    }

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send, ResponseData &data) {
        auto target = std::string(req.target());
        http::basic_fields<Allocator> tmp;

        if (req.target().starts_with("/api/")) {
            std::make_shared<StrandAPIRequest<Body, Allocator, Send>>(
                strand_, std::forward<decltype(req)>(req), std::forward<decltype(send)>(send),
                target, data, api_handler_, game_)->Execute();
        }
        else {
            HandleStaticContentRequest(std::move(req), std::move(send), target, data);
        }
    }

private:
    net::strand<net::io_context::executor_type> &strand_;
    model::Game& game_;
    fs::path static_path_;
    std::string static_folder_str_;
    API_Handler api_handler_;

    template <typename Body, typename Allocator, typename Send>
    void HandleStaticContentRequest(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send, std::string target, ResponseData &resp_data) {
        Response response_maker_;
        // 1. Transform URL from target to normal string.
        target = DecodeURL(target);
        if (target == "/") {
            target = "/index.html";
        }

        fs::path target_path = StringToPath(static_folder_str_ + target);

        // 2. Check is target in static
        if (IsSubPath(target_path, static_path_)) {
            // 3. Return file with responce if exist
            if (fs::exists(target_path)) {
                std::string file_extension {target_path.extension().c_str()};

                http::response<http::file_body> response;
                response.version(req.version());
                response.result(http::status::ok);
                response.insert(http::field::content_type, response_maker_.GetTypeByFileExtention(file_extension));

                http::file_body::value_type file_to_return;

                if (sys::error_code ec; file_to_return.open(target_path.c_str(), beast::file_mode::read, ec), ec) {
                    boost::json::value custom_data{{"filename"s, target_path.c_str()}, {"address"s, "0.0.0.0"s}};
                    logger::LogJSON(custom_data, "Error opening file"sv);
                }

                response.body() = std::move(file_to_return);
                response.prepare_payload();

                resp_data.status = http::status::ok;
                resp_data.content_type = response_maker_.GetTypeByFileExtention(file_extension);
                send(response);
            }
            else {
                // return 404 Not Found
                std::string body = "No file with path: " + target;

                resp_data.status = http::status::not_found;
                resp_data.content_type = Response::ContentType::TEXT_PLAIN;
                send(response_maker_.MakeStringResponse(http::status::not_found,
                    body, req.version(), req.keep_alive(), Response::AllowData::EMPTY, Response::ContentType::TEXT_PLAIN));
            }
        } else {
            // return 400 Bad Request
            std::string body = "Trying to leave STATIC folder with target: " + target;

            resp_data.status = http::status::bad_request;
            resp_data.content_type = Response::ContentType::TEXT_PLAIN;
            send(response_maker_.MakeStringResponse(http::status::bad_request,
                body, req.version(), req.keep_alive(), Response::AllowData::EMPTY, Response::ContentType::TEXT_PLAIN));
        }
    }
};

class LoggingRequestHandler: public RequestHandler {
    using tcp = net::ip::tcp;

    template <typename Body, typename Allocator>
    static void LogRequest(const http::request<Body, http::basic_fields<Allocator>>& req, const tcp::endpoint& end_point) {
        json::value custom_data{
            {"ip"s, end_point.address().to_string()},
            {"URI"s, req.target()},
            {"method"s, req.method_string()}
        };

        logger::LogJSON(custom_data, "request received"sv);
    }

    template <typename Send>
    static void LogResponse(const Send& res, const tcp::endpoint& end_point, ResponseData& data, int responce_time) {
        json::value custom_data{
            {"ip"s, end_point.address().to_string()},
            {"response_time"s, responce_time},
            {"code"s, static_cast<int>(data.status) },
            {"content_type"s, data.content_type}
        };

        logger::LogJSON(custom_data, "response sent"sv);
    }

public:
    LoggingRequestHandler(net::strand<net::io_context::executor_type> &strand, model::Game& game, std::string static_folder)
        :RequestHandler{strand, game, static_folder} {
    }

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send, const tcp::endpoint& end_point) {
        ResponseData data;

        LogRequest(req, end_point);

        steady_clock::time_point start_time{steady_clock::now()};
        RequestHandler::operator()(std::forward<decltype(req)>(req), std::forward<decltype(send)>(send), data);

        LogResponse(send, end_point, data, std::chrono::duration_cast<std::chrono::milliseconds>(steady_clock::now() - start_time).count());
    }
};

}  // namespace http_handler

#endif