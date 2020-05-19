#define RAPIDJSON_HAS_STDSTRING 1

#include "../include/main.hpp"
#include "../extern/beatsaber-hook/shared/utils/il2cpp-utils.hpp"
#include "../extern/beatsaber-hook/shared/config/config-utils.hpp"
#include "../extern/questui/AssetBundle.hpp"
#include "../extern/questui/questui.hpp"
#include "../extern/TwitchIRC/TwitchIRCClient.hpp"

#include <dlfcn.h>
#include <chrono>
#include <map>
#include <queue>
#include <thread>

static bool boolTrue = true;
static bool boolFalse = false;

static auto& config_doc = Configuration::config;

static TwitchIRCClient* client = nullptr;
static std::map<std::string, std::string> usersColorCache;

static AssetBundle* assetBundle = nullptr;
static Il2CppObject* customUIObject = nullptr;
static Il2CppObject* chatObject_Template = nullptr;
static std::deque<ChatObject*> chatObjects;
static std::queue<ChatObject*> chatObjectsToAdd;
static const int maxVisibleObjects = 24;

static long long lastUpdate = 0;
static bool needUpdate = false;
static bool threadStarted = false;
static bool isInMenu = false;

static bool isLoadingAsset = false;
static bool reloadAsset = false;

void UpdateList() {
    if (!chatObjectsToAdd.empty()) needUpdate = true;
    long long currentTime =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    if (chatObject_Template != nullptr && needUpdate && currentTime - lastUpdate > 100) {
        needUpdate = false;
        lastUpdate = currentTime;
        if (!chatObjectsToAdd.empty()) {
            chatObjects.push_back(chatObjectsToAdd.front());
            chatObjectsToAdd.pop();
        }
        for (int i = 0; i < chatObjects.size(); i++) {
            ChatObject* chatObject = chatObjects[i];
            if (chatObject->GameObject == nullptr) {
                ChatObject* lastChatObject = nullptr;
                if (i > 0) lastChatObject = chatObjects[i - 1];
                // Vector3 lastChatObjectPosition;
                // Vector3 lastChatObjectSize;
                // if (lastChatObject && lastChatObject->GameObject) {
                //     lastChatObjectPosition =
                //         CRASH_UNLESS(il2cpp_utils::GetPropertyValue<Vector3>(lastChatObject->GameObject, "localPosition"));
                //     lastChatObjectSize =
                //         CRASH_UNLESS(il2cpp_utils::GetPropertyValue<Vector3>(lastChatObject->GameObject, "sizeDelta"));
                // }
                chatObject->GameObject =
                    CRASH_UNLESS(il2cpp_utils::RunMethod("UnityEngine", "Object", "Instantiate", chatObject_Template));
                CRASH_UNLESS(
                    il2cpp_utils::SetPropertyValue(chatObject->GameObject, "name", il2cpp_utils::createcsstr("ChatObject")));
                UnityHelper::SetSameParent(chatObject->GameObject, chatObject_Template);
                UnityHelper::SetGameObjectActive(chatObject->GameObject, true);
                UnityHelper::SetButtonText(chatObject->GameObject, chatObject->Text);
            }
        }
        while (maxVisibleObjects < chatObjects.size()) {
            ChatObject* chatObject = chatObjects.front();
            if (chatObject->GameObject != nullptr)
                il2cpp_utils::RunMethod("UnityEngine", "Object", "Destroy", UnityHelper::GetGameObject(chatObject->GameObject));
            chatObjects.pop_front();
            delete chatObject;
        }
    }
}

void AddChatObject(std::string&& text) {
    ChatObject* chatObject = new ChatObject();
    chatObject->Text = text;
    chatObjectsToAdd.push(chatObject);
}

void OnChatMessage(IRCMessage message, TwitchIRCClient* client) {
    if (Config.Blacklist.count(message.prefix.nick)) {
        log(INFO, "Twitch Chat: Blacklisted user %s sent the message: %s", message.prefix.nick.c_str(),
            message.parameters.at(message.parameters.size() - 1).c_str());
        return;
    } else {
        log(INFO, "Twitch Chat: user %s sent the message: %s", message.prefix.nick.c_str(),
            message.parameters.at(message.parameters.size() - 1).c_str());
    }
    if (usersColorCache.find(message.prefix.nick) == usersColorCache.end())
        usersColorCache.emplace(message.prefix.nick, int_to_hex(rand() % 0x1000000, 6));
    std::string text = "<color=" + usersColorCache[message.prefix.nick] + ">" + message.prefix.nick +
                       "</color>: " + message.parameters.at(message.parameters.size() - 1);
    log(INFO, "Twitch Chat: %s", text.c_str());
    AddChatObject(std::move(text));
}

void TwitchIRCThread() {
    client = new TwitchIRCClient();
    if (client->InitSocket()) {
        if (client->Connect()) {
            if (client->Login("justinfan" + std::to_string(1030307 + rand() % 1030307), "xxx")) {
                log(INFO, "Twitch Chat: Logged In!");
                client->HookIRCCommand("PRIVMSG", OnChatMessage);
                if (client->JoinChannel(Config.Channel)) {
                    log(INFO, "Twitch Chat: Joined Channel %s!", Config.Channel.c_str());
                    AddChatObject("<color=#55FF00FF>Joined Channel:</color> <color=#FFCC00FF>" + Config.Channel + "</color>");
                    while (client->Connected()) {
                        client->ReceiveData();
                    }
                }
            }
            AddChatObject("<color=#FF0000FF>Disconnected!</color>");
            client->Disconnect();
            log(INFO, "Twitch Chat: Disconnected!");
        }
    }
}

void OnLoadAssetComplete(Asset* asset) {
    customUIObject = CRASH_UNLESS(il2cpp_utils::RunMethod("UnityEngine", "Object", "Instantiate", asset));
    auto* objectTransform = CRASH_UNLESS(il2cpp_utils::GetPropertyValue(customUIObject, "transform"));

    Vector3 scale;
    if (isInMenu) {
        il2cpp_utils::SetPropertyValue(objectTransform, "position", Config.PositionMenu);
        il2cpp_utils::SetPropertyValue(objectTransform, "eulerAngles", Config.RotationMenu);
        scale = Config.ScaleMenu;
    } else {
        il2cpp_utils::SetPropertyValue(objectTransform, "position", Config.PositionGame);
        il2cpp_utils::SetPropertyValue(objectTransform, "eulerAngles", Config.RotationGame);
        scale = Config.ScaleGame;
    }
    scale.x *= 0.0025f;
    scale.y *= 0.0025f;
    scale.z *= 0.0025f;
    il2cpp_utils::SetPropertyValue(objectTransform, "localScale", scale);

    UnityHelper::SetGameObjectActive(customUIObject, true);
    auto* tRectT = CRASH_UNLESS(il2cpp_utils::GetSystemType("UnityEngine", "RectTransform"));
    chatObject_Template = UnityHelper::GetComponentInChildren(customUIObject, tRectT, "ChatObject_Template");
    RET_V_UNLESS(chatObject_Template);
    UnityHelper::SetGameObjectActive(chatObject_Template, false);
    needUpdate = true;
    if (!threadStarted) {
        threadStarted = true;
        std::thread twitchIRCThread(TwitchIRCThread);
        twitchIRCThread.detach();
    }
}

void OnLoadAssetBundleComplete(AssetBundle* assetBundleArg) {
    assetBundle = assetBundleArg;
    assetBundle->LoadAssetAsync("_customasset", OnLoadAssetComplete);
}

MAKE_HOOK_OFFSETLESS(Camera_FireOnPostRender, void, Il2CppObject* cam) {
    Camera_FireOnPostRender(cam);
    UpdateList();
}

MAKE_HOOK_OFFSETLESS(SceneManager_SetActiveScene, bool, Scene scene) {
    if (customUIObject) {
        chatObject_Template = nullptr;
        for (int i = 0; i < chatObjects.size(); i++) {
            chatObjects[i]->GameObject = nullptr;
        }
        CRASH_UNLESS(il2cpp_utils::RunMethod("UnityEngine", "Object", "Destroy", customUIObject));
        customUIObject = nullptr;
        log(INFO, "Destroyed ChatUI!");
    }
    auto* nameObject = CRASH_UNLESS(il2cpp_utils::GetPropertyValue<Il2CppString*>(scene, "name"));
    const char* name = to_utf8(csstrtostr(nameObject)).c_str();
    log(INFO, "Scene: %s", name);
    isInMenu = strcmp(name, "MenuViewControllers") == 0;
    if (isInMenu || strcmp(name, "GameCore") == 0) {
        const char* assetBundlePath = "/sdcard/Android/data/com.beatgames.beatsaber/files/uis/chatUI.qui";

        if (!assetBundle) {
            log(DEBUG, "AssetBundle: Loading from chatUI.qui");
            AssetBundle::LoadFromFileAsync(assetBundlePath, OnLoadAssetBundleComplete);
        } else {
            // Load the asset again
            OnLoadAssetBundleComplete(assetBundle);
        }
    }
    return SceneManager_SetActiveScene(scene);
}

void AddChildVector(ConfigValue& parent, std::string_view vecName, Vector3& vec, ConfigDocument::AllocatorType& allocator) {
    ConfigValue value(rapidjson::kObjectType);
    value.AddMember("X", vec.x, allocator);
    value.AddMember("Y", vec.y, allocator);
    value.AddMember("Z", vec.z, allocator);
    parent.AddMember((ConfigValue::StringRefType)vecName.data(), value, allocator);
}

void SaveConfig() {
    log(INFO, "Saving Configuration...");
    config_doc.RemoveAllMembers();
    config_doc.SetObject();
    auto& allocator = config_doc.GetAllocator();

    config_doc.AddMember("Channel", Config.Channel, allocator);

    ConfigValue menuValue(rapidjson::kObjectType);
    AddChildVector(menuValue, "Position", Config.PositionMenu, allocator);
    AddChildVector(menuValue, "Rotation", Config.RotationMenu, allocator);
    AddChildVector(menuValue, "Scale", Config.ScaleMenu, allocator);
    config_doc.AddMember("Menu", menuValue, allocator);

    ConfigValue gameValue(rapidjson::kObjectType);
    AddChildVector(gameValue, "Position", Config.PositionGame, allocator);
    AddChildVector(gameValue, "Rotation", Config.RotationGame, allocator);
    AddChildVector(gameValue, "Scale", Config.ScaleGame, allocator);
    config_doc.AddMember("Game", gameValue, allocator);

    ConfigValue blacklist(rapidjson::kArrayType);
    for (auto& name : Config.Blacklist) {
        ConfigValue value(name.c_str(), name.size());
        blacklist.PushBack(value, allocator);
    }
    config_doc.AddMember("Blacklist", blacklist, allocator);

    Configuration::Write();
    log(INFO, "Saved Configuration!");
}

bool ParseVector(Vector3& vec, ConfigValue& parent, std::string_view vecName) {
    if (!parent.HasMember(vecName.data()) || !parent[vecName.data()].IsObject()) return false;
    ConfigValue value = parent[vecName.data()].GetObject();
    if (!(value.HasMember("X") && value["X"].IsFloat() &&
          value.HasMember("Y") && value["Y"].IsFloat() &&
          value.HasMember("Z") && value["Z"].IsFloat())) return false;
    vec.x = value["X"].GetFloat();
    vec.y = value["Y"].GetFloat();
    vec.z = value["Z"].GetFloat();
    return true;
}

bool LoadConfig() {
    log(INFO, "Loading Configuration...");
    Configuration::Load();
    bool foundEverything = true;

    if (config_doc.HasMember("Channel") && config_doc["Channel"].IsString()) {
        std::string data(config_doc["Channel"].GetString());
        Config.Channel = data;
    } else foundEverything = false;

    if (config_doc.HasMember("Menu") && config_doc["Menu"].IsObject()) {
        ConfigValue menuValue = config_doc["Menu"].GetObject();
        if (!ParseVector(Config.PositionMenu, menuValue, "Position")) foundEverything = false;
        if (!ParseVector(Config.RotationMenu, menuValue, "Rotation")) foundEverything = false;
        if (!ParseVector(Config.ScaleMenu, menuValue, "Scale")) foundEverything = false;
    } else foundEverything = false;

    if (config_doc.HasMember("Game") && config_doc["Game"].IsObject()) {
        ConfigValue gameValue = config_doc["Game"].GetObject();
        if (!ParseVector(Config.PositionGame, gameValue, "Position")) foundEverything = false;
        if (!ParseVector(Config.RotationGame, gameValue, "Rotation")) foundEverything = false;
        if (!ParseVector(Config.ScaleGame, gameValue, "Scale")) foundEverything = false;
    } else foundEverything = false;

    Config.Blacklist.clear();
    if (config_doc.HasMember("Blacklist") && config_doc["Blacklist"].IsArray()) {
        for (rapidjson::SizeType i = 0; i < config_doc["Blacklist"].Size(); i++) {
            if (config_doc["Blacklist"][i].IsString()) {
                Config.Blacklist.insert(config_doc["Blacklist"][i].GetString());
            }
        }
    } else {
        Config.Blacklist.insert("dootybot");
        Config.Blacklist.insert("nightbot");
        foundEverything = false;
    }

    if (foundEverything) {
        log(INFO, "Loaded Configuration!");
    }
    return foundEverything;
}

extern "C" void load() {
    if (!LoadConfig()) SaveConfig();
    log(INFO, "Starting ChatUI installation...");
    il2cpp_functions::Init();
    INSTALL_HOOK_OFFSETLESS(Camera_FireOnPostRender,
        il2cpp_utils::FindMethodUnsafe("UnityEngine", "Camera", "FireOnPostRender", 1));
    INSTALL_HOOK_OFFSETLESS(SceneManager_SetActiveScene,
        il2cpp_utils::FindMethodUnsafe("UnityEngine.SceneManagement", "SceneManager", "SetActiveScene", 1));

    log(INFO, "Successfully installed ChatUI!");
}
