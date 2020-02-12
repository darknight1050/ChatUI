#include <dlfcn.h>
#include <vector>
#include <map>
#include <thread>
#include <chrono>
#include <iomanip>
#include "../extern/beatsaber-hook/shared/utils/utils.h"
#include "../extern/questui/questui.hpp"
#include "../extern/TwitchIRC/TwitchIRCClient.hpp"

#define RAPIDJSON_HAS_STDSTRING 1

using namespace std;

static bool boolTrue = true;
static bool boolFalse = false;

static struct Config_t {
    string Nick = "";
    string OAuth = "";
    string Channel = "";
    Vector3 PositionMenu = {0.0f, 4.4f, 4.0f};
    Vector3 RotationMenu = {-36.0f, 0.0f, 0.0f};
    Vector3 ScaleMenu = {1.0f, 1.0f, 1.0f};
    Vector3 PositionGame = {0.0f, 4.0f, 4.0f};
    Vector3 RotationGame = {-36.0f, 0.0f, 0.0f};
    Vector3 ScaleGame = {1.0f, 1.0f, 1.0f};
    vector<string>* Blacklist = nullptr;
} Config;

struct ChatObject {
    string Text;
    Il2CppObject* GameObject;
};

template <typename T>
inline string int_to_hex(T val, size_t width=sizeof(T)*2)
{
    stringstream ss;
    ss << "#" << setfill('0') << setw(width) << hex << (val|0) << "ff";
    return ss.str();
}

__attribute__((constructor)) void lib_main();

