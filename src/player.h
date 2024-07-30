#ifndef __PLAYER__
#define __PLAYER__

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

#include "map.h"

namespace model {

enum Direction {
        NORTH,
        SOUTH,
        WEST,
        EAST,
        NONE
    };

inline static std::unordered_map<std::string, Direction> StringToDir = {
    { "U", Direction::NORTH },
    { "D", Direction::SOUTH },
    { "L", Direction::WEST },
    { "R", Direction::EAST },
    { "", Direction::NONE }
};

inline static std::unordered_map<Direction, std::string> DirToString = {
    {Direction::NORTH, "U"},
    {Direction::SOUTH, "D"},
    {Direction::WEST, "L"},
    {Direction::EAST, "R"},
    {Direction::NONE, ""}
};

struct DogSpeed {
    double x = 0.0f;
    double y = 0.0f;
};

class DogCoord {
public:
    DogCoord(double x, double y):
        point_({x, y}) {}

    void SetCoords(double x, double y) {
        point_ = { x, y };
    }

    void SetDirection(Direction new_direct) {
        curent_direction_ = new_direct;
    }

    DogPoint GetCoords() const {
        return point_;
    }

    std::string GetDirection() const {
        return DirToString.at(curent_direction_);
    }

private:
    DogPoint point_;
    Direction curent_direction_ = Direction::NORTH;
};

class Dog {
public:
    Dog(double x, double y):
        coords_(model::DogCoord(x, y)) {}

    std::string GetDogData() const;
    void MoveDog(std::string new_direction, double speed);
    bool IsOnMove();
    void SetCoords(double x, double y);
    void SetSpeed(double x, double y);
    DogPoint GetCurrentPoint() const;
    DogSpeed GetCurrentSpeed() const;

private:
    DogCoord coords_;
    DogSpeed speed_;

    void MoveDog(Direction new_direction, double speed);
};

class Player {
public:
    Player(int id, std::string uname, std::pair<double, double> coords):
        id_(id), username_(uname), dog_(coords.first, coords.second) {}

    std::string GetName() const;
    int GetId() const;
    std::string GetDogCoords() const;
    void MoveDog(std::string new_direction, double speed);
    void UpdateState(int tick_rate, const Map* map);

private:
    int id_; 
    std::string username_;
    Dog dog_;
};

} // namespace model

#endif