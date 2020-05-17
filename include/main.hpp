#ifndef MAIN_H
#define MAIN_H

#pragma once
#include <dlfcn.h>
#include <vector>
#include <map>
#include <thread>
#include <chrono>
#include <iomanip>
#include "../extern/beatsaber-hook/shared/utils/utils.h"
#include "../extern/questui/questui.hpp"
#include "../extern/TwitchIRC/TwitchIRCClient.hpp"
#include "../extern/CustomSabers/AssetImporter.hpp"

#define RAPIDJSON_HAS_STDSTRING 1

static bool boolTrue = true;
static bool boolFalse = false;

static struct Config_t {
    std::string Channel = "";
    Vector3 PositionMenu = {0.0f, 4.4f, 4.0f};
    Vector3 RotationMenu = {-36.0f, 0.0f, 0.0f};
    Vector3 ScaleMenu = {1.0f, 1.0f, 1.0f};
    Vector3 PositionGame = {0.0f, 4.0f, 4.0f};
    Vector3 RotationGame = {-36.0f, 0.0f, 0.0f};
    Vector3 ScaleGame = {1.0f, 1.0f, 1.0f};
    std::vector<std::string>* Blacklist = nullptr;
} Config;

struct ChatObject {
    std::string Text;
    Il2CppObject* GameObject;
};

template <typename T>
inline std::string int_to_hex(T val, size_t width=sizeof(T)*2)
{
    std::stringstream ss;
    ss << "#" << std::setfill('0') << std::setw(width) << std::hex << (val|0) << "ff";
    return ss.str();
}

#endif
