#include "player.h"

namespace model {

// ------------------------------ Dog ------------------------------
std::string Dog::GetDogData() const {
    auto coords = coords_.GetCoords();
    std::string data = "";

    data += "\t \"pos\": [" + std::to_string(coords.x) + ", " + std::to_string(coords.y) + "],\n";
    data += "\t \"speed\": [" + std::to_string(speed_.x) + ", " + std::to_string(speed_.y) + "],\n";
    data += "\t \"dir\": \"" + coords_.GetDirection() + "\" \n";
    return data;
}

void Dog::MoveDog(std::string new_direction, double speed) {
    MoveDog(StringToDir.at(new_direction), speed);
}

bool Dog::IsOnMove() {
    return speed_.x != 0.0f || speed_.y != 0.0f;
}

void Dog::SetCoords(double x, double y) {
    coords_.SetCoords(x, y);
}

void Dog::SetSpeed(double x, double y) {
    speed_.x = x;
    speed_.y = y;
}

DogPoint Dog::GetCurrentPoint() const {
    return coords_.GetCoords();
}

DogSpeed Dog::GetCurrentSpeed() const {
    return speed_;
}

void Dog::MoveDog(Direction new_direction, double speed) {
    if (new_direction != Direction::NONE) {
        coords_.SetDirection(new_direction);
    }

    switch (new_direction) {
    case Direction::NORTH :
        speed_.x = 0.0f;
        speed_.y = speed * -1;
        break;
    case Direction::SOUTH :
        speed_.x = 0.0f;
        speed_.y = speed;
        break;
    case Direction::WEST :
        speed_.x = speed * -1;
        speed_.y = 0.0f;
        break;
    case Direction::EAST :
        speed_.x = speed;
        speed_.y = 0.0f;
        break;
    default:
        speed_.x = 0.0f;
        speed_.y = 0.0f;
    }
}

// ------------------------------ Player ------------------------------
std::string Player::GetName() const {
    return username_;
}

int Player::GetId() const {
    return id_;
}

std::string Player::GetDogCoords() const {
    return dog_.GetDogData();
}

void Player::MoveDog(std::string new_direction, double speed) {
    dog_.MoveDog(new_direction, speed);
}

void Player::UpdateState(int tick_rate, const Map* map) {
    if (!dog_.IsOnMove()) { return; }

    auto current_point = dog_.GetCurrentPoint();
    double x = (current_point.x + dog_.GetCurrentSpeed().x * tick_rate * 0.001);
    double y = (current_point.y + dog_.GetCurrentSpeed().y * tick_rate * 0.001);

    if (map->IsPointOnRoad(current_point, {x, y})) {
        dog_.SetCoords(x, y);
    } else {
        auto new_coords = map->HandleCollizion(current_point, {x, y});
        if (new_coords.x != x || new_coords.y != y) {
            dog_.SetSpeed(0.0, 0.0);
        }
        dog_.SetCoords(new_coords.x, new_coords.y);
    }
}

} // namespace model