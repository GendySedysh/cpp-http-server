#ifndef __MODEL__
#define __MODEL__

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

#include "http_server.h"
#include "tagged.h"
#include "map.h"
#include "player.h"

namespace model {

class GameSession {
public:
    GameSession (const Map *map):
        map_{map} {}

    std::pair<int, std::string> AddPlayer(int id, std::string username, bool is_random) {
        std::string token = token_generator_.GenerateToken();

        std::pair<double, double> spawn_point = map_->GetDefaultPoint();
        if (is_random) {
            spawn_point = map_->GetRandomRoadPoint();
        }
        Player new_player(id, username, spawn_point);
        token_to_player_.insert( {token, new_player} );
        return { new_player.GetId(), token };
    }

    void MovePlayerWithToken(std::string token, std::string direction) {
        Player &player = token_to_player_.at(token);

        player.MoveDog(direction, map_->GetDogSpeed());
    }

    bool IsTokenInSession(std::string token) const {
        return (token_to_player_.count(token) != 0);
    }

    std::string GetPlayerList() const {
        pt::ptree data;

        for (auto &[token, player]: token_to_player_) {
            pt::ptree player_ptree;

            player_ptree.put("name", player.GetName());
            data.push_back(std::make_pair(std::to_string(player.GetId()), player_ptree));
        }

        std::ostringstream oss;
        bpt::write_json(oss, data);
        return oss.str();
    }

    std::string GetPlayerData() {
        std::string data = "{\"players\": {\n";

        int i = 0;
        for (const auto &[token, player]: token_to_player_) {
            data += "\t\"" + std::to_string(player.GetId()) + "\": {" + player.GetDogCoords() + "}";
            ++i;
            (i == token_to_player_.size()) ? data += "\n" : data += ",\n";
        }
        data += "}\n}";
        return data;
    }

    void UpdateState(int tick_rate) {
        for (auto &[token, player]: token_to_player_) {
            player.UpdateState(tick_rate, map_);
        }
    }

private:
    class TokenGenerator {
    public:
        std::string GenerateToken() {
            std::stringstream stream;
    
            std::uniform_int_distribution<std::mt19937_64::result_type> dist;
            stream << std::hex << dist(generator1_) << dist(generator2_);
            std::string token = stream.str();
            
            while (token.length() != 32) {
                token += "0";
            }
            return token;
        }

    private:
        // Чтобы сгенерировать токен, получите из generator1_ и generator2_
        // два 64-разрядных числа и, переведя их в hex-строки, склейте в одну.
        std::mt19937 generator1_{std::random_device{}()};
        std::mt19937 generator2_{std::random_device{}()};
    };

    std::unordered_map<std::string, Player> token_to_player_;
    const Map* map_;
    TokenGenerator token_generator_;
};

class Game {
public:
    using Maps = std::vector<Map>;

    void AddMap(Map map);

    GameSession &GetGameSession(const Map::Id& id) {
        if (map_id_to_session_.count(id) == 0) {
            AddGameSession(id);
        }

        return map_id_to_session_.at(id);
    }

    bool HaveGameSessionWithToken(const std::string &token) {
        for (auto &[id, game_session]: map_id_to_session_) {
            if (game_session.IsTokenInSession(token)) {
                return true;
            }
        }
        return false;
    }

    GameSession &GetGameSessionByToken(const std::string &token) {
        for (auto &[id, game_session]: map_id_to_session_) {
            if (game_session.IsTokenInSession(token)) {
                return GetGameSession(id);
            }
        }
        return GetGameSession(Map::Id("1"));
    }

    std::pair<int, std::string> AddPlayerToSession(GameSession &game_session, std::string &username) {
        return game_session.AddPlayer(player_id_++, username, randomize_player_spawn);
    }

    const Maps& GetMaps() const noexcept {
        return maps_;
    }

    const Map* FindMap(const Map::Id& id) const noexcept {
        if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
            return &maps_.at(it->second);
        }
        return nullptr;
    }

    int GetTickrate() const {
        return tickrate_;
    }

    void SetTickrate(int rate) {
        tickrate_ = rate;
    }

    void UpdateStates() {
        for (auto &[id, game_session]: map_id_to_session_) {
            game_session.UpdateState(tickrate_);
        }
    }

    void SetPlayerSpawn(bool type) {
        randomize_player_spawn = type;
    }

private:
    using MapIdHasher = util::TaggedHasher<Map::Id>;
    using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;

    void AddGameSession(const Map::Id& id) {
        GameSession session(FindMap(id));
        map_id_to_session_.insert(std::make_pair(id, session));
    }

    std::vector<Map> maps_;
    MapIdToIndex map_id_to_index_;
    std::unordered_map<Map::Id, GameSession, MapIdHasher> map_id_to_session_;
    bool randomize_player_spawn = false;
    int tickrate_ = 0;
    int player_id_ = 0;
};

}  // namespace model

#endif