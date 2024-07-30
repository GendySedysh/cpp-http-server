#include "map.h"

namespace model {

using Id = util::Tagged<std::string, Map>;
using Roads = std::vector<Road>;
using Buildings = std::vector<Building>;
using Offices = std::vector<Office>;

double RoundToOnePoint(double x) {
   return static_cast<double>(x * 10.) / 10.;
}

std::string PtreeToString(pt::ptree data) {
    return boost::property_tree::write_jsonEx(data);
}

double FindDistance(DogPoint start, DogPoint end) {
    return std::sqrt((end.x - start.x)*(end.x - start.x) + (end.y - start.y)*(end.y - start.y));
}
    
// ------------------------------ Road ------------------------------
bool Road::IsHorizontal() const noexcept {
    return start_.y == end_.y;
}

bool Road::IsVertical() const noexcept {
    return start_.x == end_.x;
}

Point Road::GetStart() const noexcept {
    return start_;
}

Point Road::GetEnd() const noexcept {
    return end_;
}

std::pair<double, double> Road::GetRandomPoint() const {
    if (IsHorizontal()) {
        int road_length = std::abs(end_.x - start_.x);
        int start_point = (end_.x < start_.x) ? end_.x : start_.x;
        return { static_cast<double>(start_point + (std::rand() % (road_length - 1))), static_cast<double>(start_.y) };
    }
    int road_length = std::abs(end_.y - start_.y);
    int start_point = (end_.y < start_.y) ? end_.y : start_.y;
    return { static_cast<double>(start_.x), static_cast<double>(start_point + (std::rand() % (road_length - 1))) };
}

// ------------------------------ Map ------------------------------
bool Map::IsPointOnRoad(DogPoint start, DogPoint end) const {
    return grid_.IsPointOnGrid(start, end);
}

DogPoint Map::HandleCollizion(DogPoint start, DogPoint end) const {
    return grid_.HandleCollizion(start, end);
}

const Id& Map::GetId() const noexcept {
    return id_;
}

const std::string& Map::GetName() const noexcept {
    return name_;
}

const Buildings& Map::GetBuildings() const noexcept {
    return buildings_;
}

const Roads& Map::GetRoads() const noexcept {
    return roads_;
}

const Offices& Map::GetOffices() const noexcept {
    return offices_;
}

const double& Map::GetDogSpeed() const noexcept {
    return dog_speed_;
}

void Map::SetDogSpeed(double new_speed) {
    dog_speed_ = new_speed;
}

void Map::AddRoad(const Road& road) {
    roads_.emplace_back(road);
    grid_.AddRoad(road);
}

void Map::AddBuilding(const Building& building) {
    buildings_.emplace_back(building);
}

void Map::AddOffice(Office office) {
    if (warehouse_id_to_index_.contains(office.GetId())) {
        throw std::invalid_argument("Duplicate warehouse");
    }

    const size_t index = offices_.size();
    Office& o = offices_.emplace_back(std::move(office));
    try {
        warehouse_id_to_index_.emplace(o.GetId(), index);
    } catch (...) {
        // Удаляем офис из вектора, если не удалось вставить в unordered_map
        offices_.pop_back();
        throw;
    }
}

std::pair<double, double> Map::GetRandomRoadPoint() const {
    Road road = roads_[std::rand() % roads_.size()];
    return road.GetRandomPoint();
}

std::pair<double, double> Map::GetDefaultPoint() const {
    auto point = roads_.front().GetStart();
    return {point.x, point.y};
}

pt::ptree Map::PrepareName() const {
    pt::ptree data;

    data.put("id", *id_);
    data.put("name", name_);

    return data;
}

// Array of roads data
pt::ptree Map::PrepareRoads() const {
    pt::ptree roads;

    for (auto road : roads_) {
        auto start_point = road.GetStart();
        auto end_point = road.GetEnd();

        pt::ptree tree_road;
        if (road.IsHorizontal()) {
            tree_road.put("x0", start_point.x);
            tree_road.put("y0", start_point.y);
            tree_road.put("x1", end_point.x);
        } else {
            tree_road.put("x0", start_point.x);
            tree_road.put("y0", start_point.y);
            tree_road.put("y1", end_point.y);
        }

        roads.push_back(std::make_pair("", tree_road));
    }
    return roads;
}

// Array of buildings data
pt::ptree Map::PrepareBuildings() const {
    pt::ptree buildings;

    for (auto building : buildings_) {
        auto bounds = building.GetBounds();

        pt::ptree tree_building;
        tree_building.put("x", bounds.position.x);
        tree_building.put("y", bounds.position.y);
        tree_building.put("w", bounds.size.width);
        tree_building.put("h", bounds.size.height);

        buildings.push_back(std::make_pair("", tree_building));
    }
    return buildings;
}

// Array of office data
pt::ptree Map::PrepareOffices() const {
    pt::ptree offices;
    for (auto office : offices_) {
        auto id = office.GetId();
        auto position = office.GetPosition();
        auto offset = office.GetOffset();

        pt::ptree tree_office;
        tree_office.put("id", *id);
        tree_office.put("x", position.x);
        tree_office.put("y", position.y);
        tree_office.put("offsetX", offset.dx);
        tree_office.put("offsetY", offset.dy);

        offices.push_back(std::make_pair("", tree_office));
    }

    return offices;
}

std::string Map::PrintMap() const {
    pt::ptree data;

    data.put("id", *id_);
    data.put("name", name_);
    data.add_child("roads", PrepareRoads());
    data.add_child("buildings", PrepareBuildings());
    data.add_child("offices", PrepareOffices());

    return PtreeToString(data);
}

// ------------------------------ RoadBounces ------------------------------
DogPoint RoadBounces::FindCollision(DogPoint start, DogPoint end) const {
    // horizontal movement
    if (start.y == end.y) {
        // left bound crossed
        if (end.x <= x1) {
            return {x1, end.y};
        }

        // right bound crossed
        if (end.x >= x2) {
            return {x2, end.y};
        }
    }

    if (start.x == end.x) {
        // bottom bound crossed
        if (end.y <= y1) {
            return {end.x, y1};
        }

        // top bound crossed
        if (end.y >= y2) {
            return {end.x, y2};
        }
    }
    return end;
}

// ------------------------------ RoadGrid ------------------------------
void RoadGrid::AddRoad(const Road& road) {
    double x1;
    double y1;
    double x2;
    double y2;

    auto start = road.GetStart();
    auto end = road.GetEnd();

    if (start.x < end.x) {
        x1 = static_cast<double>(start.x);
        x2 = static_cast<double>(end.x);
    } else {
        x1 = static_cast<double>(end.x);
        x2 = static_cast<double>(start.x);
    }

    if (start.y < end.y) {
        y1 = static_cast<double>(start.y);
        y2 = static_cast<double>(end.y);
    } else {
        y1 = static_cast<double>(end.y);
        y2 = static_cast<double>(start.y);
    }

    roads_.push_back({ x1 - 0.4, y1 - 0.4, x2 + 0.4 ,y2 + 0.4 });
}

// Идея в том, что если собака стоит на перекрёстке просчитать все доступные валидные точки 
// и выбрать ту которая ближе к конечной 
DogPoint RoadGrid::HandleCollizion(DogPoint start, DogPoint end) const {
    std::vector<DogPoint> points;

    std::vector<RoadBounces> grids = GetAllGrids(start);
    std::transform(grids.begin(), grids.end(), std::back_inserter(points), [start, end](const RoadBounces& grid) {
        return grid.FindCollision(start, end);
    });
    
    // сортируем полученные точки по близости к конечной точке
    std::sort(points.begin(), points.end(), [end](DogPoint lhs, DogPoint rhs){
        return FindDistance(lhs, end) < FindDistance(rhs, end);
    });

    return points.front();
}

int RoadGrid::CountPointWithGrid(DogPoint point) const {
    return std::count_if(roads_.begin(), roads_.end(), [&point](const auto &bound){
        return (point.x >= bound.x1 && point.x <= bound.x2) && (point.y >= bound.y1 && point.y <= bound.y2);
    });
}

std::vector<RoadBounces> RoadGrid::GetAllGrids(DogPoint point) const {
    std::vector<RoadBounces> grids;

    std::copy_if(roads_.begin(), roads_.end(), std::back_inserter(grids), [&point](const auto &bound){
        return (point.x >= bound.x1 && point.x <= bound.x2) && (point.y >= bound.y1 && point.y <= bound.y2);
    });
    return grids;
}

const RoadBounces& RoadGrid::GetGridWithPoint(DogPoint point) const {
    for (auto &bound: roads_) {
        if ((point.x >= bound.x1 && point.x <= bound.x2) && (point.y >= bound.y1 && point.y <= bound.y2)) {
            return bound;
        }
    }
    return roads_[0];
}

bool RoadGrid::GridPoint(DogPoint point) const {
    return CountPointWithGrid(point) != 0;
}

bool RoadGrid::IsPointOnGrid(DogPoint start, DogPoint end) const {
    // конечная точка за пределами дороги
    if (!GridPoint(end)) {
        return false;
    }

    const RoadBounces &start_grid = GetGridWithPoint(start);
    const RoadBounces &end_grid = GetGridWithPoint(end);

    // либо перекрёсток, либо одна и та же дорога
    if (&start_grid == &end_grid || CountPointWithGrid(start) > 1) {
        return true;
    }
    return false;
}

} // namespace model