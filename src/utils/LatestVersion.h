//  To parse this JSON data, first install
//
//      json.hpp  https://github.com/nlohmann/json
//
//  Then include this file, and then do
//
//     LatestVersion data = nlohmann::json::parse(jsonString);

#pragma once

#include "json.hpp"

#include <optional>
#include <regex>
#include <stdexcept>

namespace AromaUpdater {
    using nlohmann::json;

#ifndef NLOHMANN_UNTYPED_AromaUpdater_HELPERHELPER
#define NLOHMANN_UNTYPED_AromaUpdater_HELPERHELPER
    inline json get_untyped(const json &j, const char *property) {
        if (j.find(property) != j.end()) {
            return j.at(property).get<json>();
        }
        return json();
    }

    inline json get_untyped(const json &j, std::string property) {
        return get_untyped(j, property.data());
    }
#endif

    class LatestVersion {
    public:
        LatestVersion()          = default;
        virtual ~LatestVersion() = default;

    private:
        std::string hash;
        std::string description;

    public:
        const std::string &getHash() const { return hash; }
        std::string &getMutableHash() { return hash; }
        void setHash(const std::string &value) { this->hash = value; }

        const std::string &getDescription() const { return description; }
        std::string &getMutableDescription() { return description; }
        void setDescription(const std::string &value) { this->description = value; }
    };
} // namespace AromaUpdater

namespace AromaUpdater {
    void from_json(const json &j, LatestVersion &x);

    inline void from_json(const json &j, LatestVersion &x) {
        x.setHash(j.at("hash").get<std::string>());
        x.setDescription(j.at("description").get<std::string>());
    }
} // namespace AromaUpdater
