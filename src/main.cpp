
#include "main.hpp"
#include "beatsaber-hook/shared/utils/utils.h"
#include "beatsaber-hook/shared/config/config-utils.hpp"

#include "TwitchIRC/TwitchIRCClient.hpp"

#include "custom-types/shared/register.hpp"

#include "questui/shared/QuestUI.hpp"

#include "UnityEngine/SceneManagement/Scene.hpp"

#include "CustomTypes/ChatHandler.hpp"
#include "ChatBuilder.hpp"
#include "customlogger.hpp"

#include <map>
#include <thread>

static ModInfo modInfo;

Config_t Config;

Logger& getLogger() {
    static auto logger = new Logger(modInfo, LoggerOptions(false, false));
    return *logger;
}

Configuration& getConfig() {
    static Configuration overall_config(modInfo);
    return overall_config;
}

TwitchIRCClient* client = nullptr;
std::map<std::string, std::string> usersColorCache;

bool threadRunning = false;

void AddChatObject(std::string&& text) {
    ChatObject chatObject = {};
    chatObject.Text = text;
    chatObject.GameObject = nullptr;
    if(chatHandler)
        chatHandler->AddChatObject(chatObject);
}

void OnChatMessage(IRCMessage message, TwitchIRCClient* client) {
    if (Config.Blacklist.count(message.prefix.nick)) {
        getLogger().info("Twitch Chat: Blacklisted user %s sent the message: %s", message.prefix.nick.c_str(),
            message.parameters.at(message.parameters.size() - 1).c_str());
        return;
    } else {
        getLogger().info("Twitch Chat: user %s sent the message: %s", message.prefix.nick.c_str(),
            message.parameters.at(message.parameters.size() - 1).c_str());
    }
    if (usersColorCache.find(message.prefix.nick) == usersColorCache.end())
        usersColorCache.emplace(message.prefix.nick, int_to_hex(rand() % 0x1000000, 6));
    std::string text = "<color=" + usersColorCache[message.prefix.nick] + ">" + message.prefix.nick +
                       "</color>: " + message.parameters.at(message.parameters.size() - 1);
    getLogger().info("Twitch Chat: %s", text.c_str());
    AddChatObject(std::move(text));
}

void TwitchIRCThread() {
    if(threadRunning) 
        return;
    threadRunning = true;
    client = new TwitchIRCClient();
    if (client->InitSocket()) {
        if (client->Connect()) {
            if (client->Login("justinfan" + std::to_string(1030307 + rand() % 1030307), "xxx")) {
                getLogger().info("Twitch Chat: Logged In!");
                client->HookIRCCommand("PRIVMSG", OnChatMessage);
                if (client->JoinChannel(Config.Channel)) {
                    getLogger().info("Twitch Chat: Joined Channel %s!", Config.Channel.c_str());
                    AddChatObject("<color=#55FF00FF>Joined Channel:</color> <color=#FFCC00FF>" + Config.Channel + "</color>");
                    while (threadRunning && client->Connected()) {
                        client->ReceiveData();
                    }
                }
            }
            AddChatObject("<color=#FF0000FF>Disconnected!</color>");
            client->Disconnect();
            getLogger().info("Twitch Chat: Disconnected!");
        }
    }
    threadRunning = false;
}

MAKE_HOOK_OFFSETLESS(SceneManager_Internal_ActiveSceneChanged, void, UnityEngine::SceneManagement::Scene prevScene, UnityEngine::SceneManagement::Scene nextScene) {
    SceneManager_Internal_ActiveSceneChanged(prevScene, nextScene);
    if(!chatGameObject) {
        if(nextScene.IsValid()) {
            std::string sceneName = to_utf8(csstrtostr(nextScene.get_name()));
            if(sceneName.find("Menu") != std::string::npos) {
                CreateChatGameObject();
                if (!threadRunning)
                    std::thread (TwitchIRCThread).detach();
            }
        }
    }
}

void AddChildVector(ConfigValue& parent, std::string_view vecName, UnityEngine::Vector3& vec, ConfigDocument::AllocatorType& allocator) {
    ConfigValue value(rapidjson::kObjectType);
    value.AddMember("X", vec.x, allocator);
    value.AddMember("Y", vec.y, allocator);
    value.AddMember("Z", vec.z, allocator);
    parent.AddMember((ConfigValue::StringRefType)vecName.data(), value, allocator);
}

void SaveConfig() {
    getLogger().info("Saving Configuration...");
    ConfigDocument& configDoc = getConfig().config;
    configDoc.RemoveAllMembers();
    configDoc.SetObject();
    auto& allocator = configDoc.GetAllocator();

    configDoc.AddMember("Channel", Config.Channel, allocator);

    ConfigValue menuValue(rapidjson::kObjectType);
    AddChildVector(menuValue, "Position", Config.PositionMenu, allocator);
    AddChildVector(menuValue, "Rotation", Config.RotationMenu, allocator);
    configDoc.AddMember("Menu", menuValue, allocator);

    ConfigValue gameValue(rapidjson::kObjectType);
    AddChildVector(gameValue, "Position", Config.PositionGame, allocator);
    AddChildVector(gameValue, "Rotation", Config.RotationGame, allocator);
    configDoc.AddMember("Game", gameValue, allocator);

    ConfigValue blacklist(rapidjson::kArrayType);
    for (auto& name : Config.Blacklist) {
        ConfigValue value(name.c_str(), name.size());
        blacklist.PushBack(value, allocator);
    }
    configDoc.AddMember("Blacklist", blacklist, allocator);

    getConfig().Write();
    getLogger().info("Saved Configuration!");
}

bool ParseVector(UnityEngine::Vector3& vec, ConfigValue& parent, std::string_view vecName) {
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
    getLogger().info("Loading Configuration...");
    getConfig().Load();
    ConfigDocument& configDoc = getConfig().config;
    bool foundEverything = true;

    if (configDoc.HasMember("Channel") && configDoc["Channel"].IsString()) {
        std::string data(configDoc["Channel"].GetString());
        Config.Channel = data;
    } else foundEverything = false;

    if (configDoc.HasMember("Menu") && configDoc["Menu"].IsObject()) {
        ConfigValue menuValue = configDoc["Menu"].GetObject();
        if (!ParseVector(Config.PositionMenu, menuValue, "Position")) foundEverything = false;
        if (!ParseVector(Config.RotationMenu, menuValue, "Rotation")) foundEverything = false;
    } else foundEverything = false;

    if (configDoc.HasMember("Game") && configDoc["Game"].IsObject()) {
        ConfigValue gameValue = configDoc["Game"].GetObject();
        if (!ParseVector(Config.PositionGame, gameValue, "Position")) foundEverything = false;
        if (!ParseVector(Config.RotationGame, gameValue, "Rotation")) foundEverything = false;
    } else foundEverything = false;

    Config.Blacklist.clear();
    if (configDoc.HasMember("Blacklist") && configDoc["Blacklist"].IsArray()) {
        for (rapidjson::SizeType i = 0; i < configDoc["Blacklist"].Size(); i++) {
            if (configDoc["Blacklist"][i].IsString()) {
                Config.Blacklist.insert(configDoc["Blacklist"][i].GetString());
            }
        }
    } else {
        Config.Blacklist.insert("dootybot");
        Config.Blacklist.insert("nightbot");
        foundEverything = false;
    }

    if (foundEverything) {
        getLogger().info("Loaded Configuration!");
    }
    return foundEverything;
}

extern "C" void setup(ModInfo& info) 
{
    modInfo.id = "ChatUI";
    modInfo.version = VERSION;
    info = modInfo;

    if (!LoadConfig()) SaveConfig();
}

extern "C" void load() {
    getLogger().info("Starting ChatUI installation...");
    il2cpp_functions::Init();

    QuestUI::Init();

    custom_types::Register::RegisterTypes<
        ChatUI::ChatHandler
        >();

    INSTALL_HOOK_OFFSETLESS(getLogger(), SceneManager_Internal_ActiveSceneChanged, il2cpp_utils::FindMethodUnsafe("UnityEngine.SceneManagement", "SceneManager", "Internal_ActiveSceneChanged", 2));
    getLogger().info("Successfully installed ChatUI!");
}