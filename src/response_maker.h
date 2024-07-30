#ifndef __RESPONCE_MAKER__
#define __RESPONCE_MAKER__

#define BOOST_BEAST_USE_STD_STRING_VIEW

#pragma once
#include <unordered_map>
#include <string_view>
#include <boost/algorithm/string.hpp>

namespace http_handler {
using namespace std::literals;

namespace beast = boost::beast;
namespace http = beast::http;

class Response{
public:
    using StringResponse = http::response<http::string_body>;

    struct AllowData {
        constexpr static std::string_view GET = "GET, HEAD"sv;
        constexpr static std::string_view POST = "POST"sv;
        constexpr static std::string_view HEAD = "HEAD"sv;
        constexpr static std::string_view EMPTY = ""sv;
    };

    struct ContentType {
        constexpr static std::string_view TEXT_HTML = "text/html"sv;
        constexpr static std::string_view TEXT_CSS = "text/css"sv;
        constexpr static std::string_view TEXT_PLAIN = "text/plain"sv;
        constexpr static std::string_view TEXT_JAVA = "text/javascript"sv;

        constexpr static std::string_view APP_JSON = "application/json"sv;
        constexpr static std::string_view APP_XML = "application/xml"sv;
        constexpr static std::string_view APP_OCTET = "application/octet-stream"sv;

        constexpr static std::string_view IMAGE_PNG = "image/png"sv;
        constexpr static std::string_view IMAGE_JPEG = "image/jpeg"sv;
        constexpr static std::string_view IMAGE_GIF = "image/gif"sv;
        constexpr static std::string_view IMAGE_BMP = "image/bmp"sv;
        constexpr static std::string_view IMAGE_ICON = "image/vnd.microsoft.icon"sv;
        constexpr static std::string_view IMAGE_TIFF = "image/tiff"sv;
        constexpr static std::string_view IMAGE_SVG = "image/svg+xml"sv;

        constexpr static std::string_view AUDIO_MPEG = "audio/mpeg"sv;

        auto GetTypeByFileExtention(std::string &extention) const {
            boost::algorithm::to_lower(extention);

            if (type_to_ext.count(extention) != 0) {
                return type_to_ext.at(extention);
            }
            return APP_OCTET;
        }
    
        std::unordered_map<std::string, const std::string_view> type_to_ext = {
            {".htm"s, TEXT_HTML}, {".html"s, TEXT_HTML},
            {".css"s, TEXT_CSS}, {".txt"s, TEXT_PLAIN},
            {".js"s, TEXT_JAVA},
            {".json"s, APP_JSON}, {".xml"s, APP_XML}, 
            {".png"s, IMAGE_PNG}, {".jpg"s, IMAGE_JPEG}, 
            {".jpeg"s, IMAGE_JPEG}, {".jpe"s, IMAGE_JPEG},
            {".gif"s, IMAGE_JPEG}, {".bmp"s, IMAGE_BMP},
            {".ico"s, IMAGE_ICON}, {".tiff"s, IMAGE_TIFF},
            {".tif"s, IMAGE_TIFF}, {".svg"s, IMAGE_SVG},
            {".svgz"s, IMAGE_SVG},
            {".mp3"s, AUDIO_MPEG}
        };
    // При необходимости внутрь ContentType можно добавить и другие типы контента
    }; // ContentType

    auto GetTypeByFileExtention(std::string &extention) {
       return content_type_.GetTypeByFileExtention(extention);
    }

    // Создаёт StringResponse с заданными параметрами
    StringResponse MakeStringResponse(
        http::status status,
        std::string_view body,
        unsigned http_version,
        bool keep_alive,
        std::string_view allow = AllowData::EMPTY,
        std::string_view content_tp = ContentType::TEXT_HTML) 
    {
        StringResponse response(status, http_version);
        response.set(http::field::content_type, content_tp);
        response.set(http::field::cache_control, "no-cache"sv);
        if (!allow.empty()) {
            response.set(http::field::allow, allow);
        }
        response.body() = body;
        response.content_length(body.size());
        response.keep_alive(keep_alive);
        return response;
    }
private:
    ContentType content_type_;
};
} // namespace http_handler
#endif