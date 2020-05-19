#pragma once

#include "../extern/beatsaber-hook/shared/utils/typedefs.h"
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_set>

static struct Config_t {
    std::string Channel = "";
    Vector3 PositionMenu = {0.0f, 4.4f, 4.0f};
    Vector3 RotationMenu = {-36.0f, 0.0f, 0.0f};
    Vector3 ScaleMenu = {1.0f, 1.0f, 1.0f};
    Vector3 PositionGame = {0.0f, 4.0f, 4.0f};
    Vector3 RotationGame = {-36.0f, 0.0f, 0.0f};
    Vector3 ScaleGame = {1.0f, 1.0f, 1.0f};
    std::unordered_set<std::string> Blacklist;
} Config;

struct ChatObject {
    std::string Text;
    Il2CppObject* GameObject;
};

template <typename T>
inline std::string int_to_hex(T val, size_t width=sizeof(T)*2) {
    std::stringstream ss;
    ss << "#" << std::setfill('0') << std::setw(width) << std::hex << (val|0) << "ff";
    return ss.str();
}
