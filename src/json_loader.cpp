#include "json_loader.h"

namespace json_loader {
namespace json = boost::json;

std::vector<model::Road> ParseRoads(json::array data) {
    std::vector<model::Road> parsed_data;

    for (auto node : data) {
        int x0 = node.at("x0").as_int64();
        int y0 = node.at("y0").as_int64();

        if (node.as_object().if_contains("x1") != NULL) {
            int x1 = node.at("x1").as_int64();
            parsed_data.emplace_back(model::Road(model::Road::HORIZONTAL, { x0, y0 }, x1));
        }
        else {
            int y1 = node.at("y1").as_int64();
            parsed_data.emplace_back(model::Road(model::Road::VERTICAL, { x0, y0 }, y1));
        }
    }
    return parsed_data;
}

std::vector<model::Building> ParseBuildings(json::array data) {
    std::vector<model::Building> parsed_data;

    for (auto node : data) {
        int x = node.at("x").as_int64();
        int y = node.at("y").as_int64();
        int w = node.at("w").as_int64();
        int h = node.at("h").as_int64();

        parsed_data.emplace_back(model::Building(model::Rectangle({x, y}, {w, h})));
    }
    return parsed_data;
}

std::vector<model::Office> ParseOffices(json::array data) {
    std::vector<model::Office> parsed_data;

    for (auto node : data) {
        model::Office::Id id( node.at("id").as_string().c_str() );
        int x = node.at("x").as_int64();
        int y = node.at("y").as_int64();
        int offsetX = node.at("offsetX").as_int64();
        int offsetY = node.at("offsetY").as_int64();

        parsed_data.emplace_back(model::Office(id, {x, y}, {offsetX, offsetY}));
    }
    return parsed_data;
}

model::Game LoadGame(const std::filesystem::path& json_path) {
    using namespace std::literals;

    model::Game game;
    std::ifstream file;

    // Загрузить содержимое файла json_path, например, в виде строки
    std::string json_data = "";
    std::string buffer;
    file.open(json_path);
    if (!file.is_open()) {
        boost::json::value custom_data{{"filename"s, json_path.string()}, {"address"s, "0.0.0.0"s}};
        logger::LogJSON(custom_data, "Error opening file"sv);
        return game;
    }

    while (std::getline(file, buffer)) {
        json_data += buffer;
    }

    auto parsed_data = json::parse(json_data);

    // пытаемся забрать defaultDogSpeed
    double defaultDogSpeed;
    try {
       defaultDogSpeed = double(parsed_data.at("defaultDogSpeed").as_double());
    } catch(...) {
        defaultDogSpeed = 1.0;
    }

    // Распарсить строку как JSON, используя boost::json::parse
    auto maps_data = parsed_data.at("maps").as_array();

    for (auto map : maps_data) {
        // пытаемся забрать dogSpeed
        double dogSpeed = defaultDogSpeed;
        try {
            dogSpeed = double(map.at("dogSpeed").as_double());
        } catch(...) { }

        // Создать карту
        model::Map::Id id( map.at("id").as_string().c_str() );
        std::string name( map.at("name").as_string().c_str() );
        model::Map new_map(id, name);

        // Добавить дороги
        auto roads = std::move(ParseRoads(map.at("roads").as_array()));
        for (const auto& road : roads) {
            new_map.AddRoad(road);
        }

        // Добавить здания
        auto buildings = std::move(ParseBuildings(map.at("buildings").as_array()));
        for (const auto& build : buildings) {
            new_map.AddBuilding(build);
        }

        // Добавить офисы
        auto offices = std::move( ParseOffices(map.at("offices").as_array()));
        for (const auto& office : offices) {
            new_map.AddOffice(office);
        }

        // Добавить скорость
        new_map.SetDogSpeed(dogSpeed);

        game.AddMap(std::move(new_map));
    }

    return game;
}

}  // namespace json_loader
