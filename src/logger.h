#ifndef __LOGGER__
#define __LOGGER__

#define BOOST_BEAST_USE_STD_STRING_VIEW

#pragma once
#include <boost/json.hpp>
#include <boost/log/core.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <string_view>
#include <iostream>

namespace logger {
    using namespace std::literals;
    using namespace boost::posix_time;
    
    namespace net = boost::asio;
    namespace sys = boost::system;

    namespace logging = boost::log;
    namespace sinks = boost::log::sinks;
    namespace keywords = boost::log::keywords;
    namespace expr = boost::log::expressions;
    namespace attrs = boost::log::attributes;

    BOOST_LOG_ATTRIBUTE_KEYWORD(additional_data, "AdditionalData", boost::json::value)
    BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime)

    inline void LogJSON(boost::json::value custom_data, std::string_view messege) {
        BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, custom_data) << messege;
    }

    inline void MyFormatter(logging::record_view const& rec, logging::formatting_ostream& strm) {
        auto ts = rec[timestamp];

        strm << "{\"timestamp\":\"" << boost::posix_time::to_iso_extended_string(*ts) << "\"";
        strm << ",\"data\":" << rec[additional_data];
        strm << ",\"message\":\"" << rec[expr::smessage] << "\"}";
    }

    inline void InitBoostLogFilter() {
        boost::log::add_console_log(std::cout, boost::log::keywords::format = &MyFormatter,
                                    logging::keywords::auto_flush = true);
        boost::log::add_common_attributes();
    }
}//logger

#endif