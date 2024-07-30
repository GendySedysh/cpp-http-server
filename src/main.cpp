#include "sdk.h"
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/bind/bind.hpp>
#include <boost/program_options.hpp>
#include <iostream>
#include <thread>
#include <format>
#include <chrono>
#include <functional>
#include <fstream>
#include <optional>
#include <vector>

#include "json_loader.h"
#include "request_handler.h"
#include "logger.h"
#include "ticker.h"

using namespace std::literals;
using namespace std::chrono;
namespace net = boost::asio;
namespace sys = boost::system;

using tcp = net::ip::tcp;
using Timer = net::steady_timer;
using Clock = std::chrono::high_resolution_clock;
using Milliseconds = std::chrono::milliseconds;

struct Args {
    int tick = 0; // uninitialize
    std::string config;
    std::string static_root;
    bool randomize_spawn = false;
};

[[nodiscard]] std::optional<Args> ParseCommandLine(int argc, const char* const argv[]) {
    namespace po = boost::program_options;

    po::options_description desc{"All options"s};

    Args args;
    desc.add_options()
        // Добавляем опцию --help и её короткую версию -h
        ("help,h", "Show help")
        ("tick-period,t", po::value<int>(&args.tick)->value_name("milliseconds"s), "set tick period")
        ("config-file,c", po::value(&args.config)->value_name("file"s), "set config file path")
        ("www-root,w", po::value(&args.static_root)->value_name("dir"s), "set static files root")
        ("randomize-spawn-points", "spawn dogs at random positions");

    // variables_map хранит значения опций после разбора
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.contains("help"s)) {
        // Если был указан параметр --help, то выводим справку и возвращаем nullopt
        std::cout << desc;
        return std::nullopt;
    }

    if (!vm.contains("config-file"s)) {
        throw std::runtime_error("config-file files have not been specified"s);
    }
    if (!vm.contains("www-root"s)) {
        throw std::runtime_error("static files root path is not specified"s);
    }
    if (!vm.contains("tick-period"s)) {
        args.tick = 0;
    }
    if (vm.contains("randomize-spawn-points")) {
        args.randomize_spawn = true;
    }

    // С опциями программы всё в порядке, возвращаем структуру args
    return args;
}

namespace {

// Запускает функцию fn на n потоках, включая текущий
template <typename Fn>
void RunWorkers(unsigned n, const Fn& fn) {
    n = std::max(1u, n);
    std::vector<std::jthread> workers;
    workers.reserve(n - 1);
    // Запускаем n-1 рабочих потоков, выполняющих функцию fn
    while (--n) {
        workers.emplace_back(fn);
    }
    fn();
}

}  // namespace


int main(int argc, const char* argv[]) {
    try {
        std::optional<Args> args = ParseCommandLine(argc, argv);
        if (!args.has_value()) {
            return EXIT_FAILURE;
        }

        // 0. Настраиваем логгер
        logger::InitBoostLogFilter();

        // 1. Загружаем карту из файла и построить модель игры
        model::Game game = json_loader::LoadGame(args.value().config);
        game.SetPlayerSpawn(args.value().randomize_spawn);
        game.SetTickrate(args.value().tick);

        // 2. Инициализируем io_context
        const unsigned num_threads = std::thread::hardware_concurrency();
        net::io_context ioc(num_threads);

        // 3. Добавляем асинхронный обработчик сигналов SIGINT и SIGTERM
        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const sys::error_code& ec, [[maybe_unused]] int signal_number) {
            if (!ec) {
                ioc.stop();

                boost::json::value custom_data{{"code"s, 0}};
                logger::LogJSON(custom_data, "server exited"sv);
            }
        });

        // 4. Создаём обработчик HTTP-запросов и связываем его с моделью игры
        net::strand<net::io_context::executor_type> strand{ net::make_strand(ioc) };
        boost::asio::steady_timer t(strand, boost::asio::chrono::milliseconds(game.GetTickrate()));

        http_handler::LoggingRequestHandler handler{strand, game, args.value().static_root};

        // 5. Запустить обработчик HTTP-запросов, делегируя их обработчику запросов
        const auto address = net::ip::make_address("0.0.0.0");
        constexpr net::ip::port_type port = 8080;

        const tcp::endpoint& end_point = {address, port};
        http_server::ServeHttp(ioc, {address, port}, [&handler, &end_point](auto&& req, auto&& send) {
            handler(std::forward<decltype(req)>(req), std::forward<decltype(send)>(send), end_point);
        });

        // Эта надпись сообщает тестам о том, что сервер запущен и готов обрабатывать запросы
        boost::json::value custom_data{{"port"s, 8080}, {"address", "0.0.0.0"}};
        logger::LogJSON(custom_data, "server started"sv);

        //Говорим игре обновлять своё состояние каждые N тиков
        int tickrate = game.GetTickrate();
        if (tickrate != 0) {
            auto ticker = std::make_shared<Ticker>(strand, std::chrono::milliseconds(tickrate),
                [&game](std::chrono::milliseconds delta) { game.UpdateStates(); }
            );
            ticker->Start();
        }

        // 6. Запускаем обработку асинхронных операций
        RunWorkers(std::max(1u, num_threads), [&ioc] {
            ioc.run();
        });
    } catch (const std::exception& ex) {
        boost::json::value custom_data{{"code"s, EXIT_FAILURE}, {"exception", ex.what()}};
        logger::LogJSON(custom_data, "server exited"sv);

        return EXIT_FAILURE;
    }
}
