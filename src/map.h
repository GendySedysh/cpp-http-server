#ifndef __MAP__
#define __MAP__

#define BOOST_BEAST_USE_STD_STRING_VIEW

#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <sstream>
#include <boost/format.hpp>
#include <boost/json.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <format>
#include <memory>
#include <random>
#include <optional>
#include <regex>

#include "http_server.h"
#include "tagged.h"


//  Штука делает вывод ЖСОН адекватной для тестов 
//  https://stackoverflow.com/questions/2855741/why-does-boost-property-tree-write-json-save-everything-as-string-is-it-possibl)

namespace bpt = boost::property_tree;
typedef bpt::ptree JSON;
namespace boost {
    namespace property_tree {
        inline std::string write_jsonEx(const JSON& ptree)
        {
            std::ostringstream oss;
            bpt::write_json(oss, ptree);
            std::regex reg("\\\"([0-9]+\\.{0,1}[0-9]*)\\\"");
            std::string result = std::regex_replace(oss.str(), reg, "$1");
            return result;
        }
    }
}

namespace model {

using Dimension = int;
using Coord = Dimension;
using boost::format;
namespace pt = boost::property_tree;

double RoundToOnePoint(double x);
std::string PtreeToString(pt::ptree data);

struct DogPoint {
    double x = 0.0f;
    double y = 0.0f;
};

double FindDistance(DogPoint start, DogPoint end);

struct Point {
    Coord x, y;
};

struct Size {
    Dimension width, height;
};

struct Rectangle {
    Point position;
    Size size;
};

struct Offset {
    Dimension dx, dy;
};

class Road {
    struct HorizontalTag {
        explicit HorizontalTag() = default;
    };

    struct VerticalTag {
        explicit VerticalTag() = default;
    };

public:
    constexpr static HorizontalTag HORIZONTAL{};
    constexpr static VerticalTag VERTICAL{};

    Road(HorizontalTag, Point start, Coord end_x) noexcept
        : start_{start}
        , end_{end_x, start.y} {
    }

    Road(VerticalTag, Point start, Coord end_y) noexcept
        : start_{start}
        , end_{start.x, end_y} {
    }

    bool IsHorizontal() const noexcept;
    bool IsVertical() const noexcept;
    Point GetStart() const noexcept;
    Point GetEnd() const noexcept;
    std::pair<double, double> GetRandomPoint() const;

private:
    Point start_;
    Point end_;
};

class Building {
public:
    explicit Building(Rectangle bounds) noexcept
        : bounds_{bounds} {
    }

    const Rectangle& GetBounds() const noexcept {
        return bounds_;
    }

private:
    Rectangle bounds_;
};

class Office {
public:
    using Id = util::Tagged<std::string, Office>;

    Office(Id id, Point position, Offset offset) noexcept
        : id_{std::move(id)}
        , position_{position}
        , offset_{offset} {
    }

    const Id& GetId() const noexcept {
        return id_;
    }

    Point GetPosition() const noexcept {
        return position_;
    }

    Offset GetOffset() const noexcept {
        return offset_;
    }

private:
    Id id_;
    Point position_;
    Offset offset_;
};

struct RoadBounces {
    double x1 = 0;
    double y1 = 0;
    double x2 = 0;
    double y2 = 0;

    DogPoint FindCollision(DogPoint start, DogPoint end) const;
};

class RoadGrid{
public:
    void AddRoad(const Road& road);
    bool IsPointOnGrid(DogPoint start, DogPoint end) const;
    DogPoint HandleCollizion(DogPoint start, DogPoint end) const;
    std::vector<RoadBounces> GetAllGrids(DogPoint point) const;
private:
    std::vector<RoadBounces> roads_;
    const RoadBounces& GetGridWithPoint(DogPoint point) const;
    int CountPointWithGrid(DogPoint point) const;
    bool GridPoint(DogPoint point) const;
};

class Map {
public:
    using Id = util::Tagged<std::string, Map>;
    using Roads = std::vector<Road>;
    using Buildings = std::vector<Building>;
    using Offices = std::vector<Office>;

    Map(Id id, std::string name) noexcept
        : id_(std::move(id))
        , name_(std::move(name)) {
    }

    
    const Id& GetId() const noexcept;
    const std::string& GetName() const noexcept;
    const Buildings& GetBuildings() const noexcept;
    const Roads& GetRoads() const noexcept;
    const Offices& GetOffices() const noexcept;
    const double& GetDogSpeed() const noexcept;

    void AddRoad(const Road& road);
    void AddBuilding(const Building& building);
    void AddOffice(Office office);

    bool IsPointOnRoad(DogPoint start, DogPoint end) const;
    DogPoint HandleCollizion(DogPoint start, DogPoint end) const;
    void SetDogSpeed(double new_speed);

    std::pair<double, double> GetRandomRoadPoint() const;
    std::pair<double, double> GetDefaultPoint() const;

    pt::ptree PrepareName() const;
    pt::ptree PrepareRoads() const;
    pt::ptree PrepareBuildings() const;
    pt::ptree PrepareOffices() const;
    std::string PrintMap() const;

private:
    using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

    Id id_;
    std::string name_;
    Roads roads_;
    Buildings buildings_;
    RoadGrid grid_;

    OfficeIdToIndex warehouse_id_to_index_;
    Offices offices_;
    double dog_speed_;
};

} // namespace model


#pragma once
#endif