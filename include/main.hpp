#pragma once

#include <iomanip>
#include <sstream>
#include <string>

#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Vector2.hpp"

struct Config_t {
    std::string Channel = "";
    UnityEngine::Vector3 PositionMenu = {0.0f, 4.4f, 4.0f};
    UnityEngine::Vector3 RotationMenu = {-36.0f, 0.0f, 0.0f};
    UnityEngine::Vector2 SizeMenu = {60.0f, 80.0f};
    float ScaleMenu = 1.0f;
    UnityEngine::Vector3 PositionGame = {0.0f, 4.0f, 4.0f};
    UnityEngine::Vector3 RotationGame = {-36.0f, 0.0f, 0.0f};
    float ScaleGame = 1.0f;
    std::unordered_set<std::string> Blacklist;
};

extern Config_t Config;

template <typename T>
inline std::string int_to_hex(T val, size_t width=sizeof(T)*2) {
    std::stringstream ss;
    ss << "#" << std::setfill('0') << std::setw(width) << std::hex << (val|0) << "ff";
    return ss.str();
}
