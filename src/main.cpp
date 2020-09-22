#define RAPIDJSON_HAS_STDSTRING 1

#include "include/main.hpp"
#include "extern/beatsaber-hook/shared/utils/il2cpp-utils.hpp"
#include "extern/beatsaber-hook/shared/config/config-utils.hpp"
#include "extern/TwitchIRC/TwitchIRCClient.hpp"


#include "UnityEngine/Transform.hpp"
#include "UnityEngine/SceneManagement/Scene.hpp"
#include "TMPro/TextMeshProUGUI.hpp"
#include "bs-utils/shared/AssetBundle.hpp"

#include <chrono>
#include <map>
#include <queue>
#include <thread>

static ModInfo modInfo;

const Logger& getLogger() {
  static const Logger& logger(modInfo);
  return logger;
}

Configuration& getConfig() {
    static Configuration overall_config(modInfo);
    return overall_config;
}
static TwitchIRCClient* client = nullptr;
static std::map<std::string, std::string> usersColorCache;

static bs_utils::AssetBundle* assetBundle = nullptr;
static UnityEngine::GameObject* customUIObject = nullptr;
static UnityEngine::GameObject* customUIMenuObject = nullptr;
static UnityEngine::GameObject* chatObject_Template = nullptr;
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
    long long currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    if (chatObject_Template && needUpdate && currentTime - lastUpdate > 100) {
        needUpdate = false;
        lastUpdate = currentTime;
        if (!chatObjectsToAdd.empty()) {
            chatObjects.push_back(chatObjectsToAdd.front());
            chatObjectsToAdd.pop();
        }
        for (int i = 0; i < chatObjects.size(); i++) {
            ChatObject* chatObject = chatObjects[i];
            if (!chatObject->GameObject) {
                chatObject->GameObject = CRASH_UNLESS(UnityEngine::GameObject::Instantiate(chatObject_Template));
                chatObject->GameObject->set_name(il2cpp_utils::createcsstr("ChatObject"));
                chatObject->GameObject->get_transform()->SetParent(chatObject_Template->get_transform()->GetParent(), false);
                chatObject->GameObject->SetActive(true);
                TMPro::TextMeshProUGUI* textObject = (TMPro::TextMeshProUGUI*)CRASH_UNLESS(chatObject->GameObject->GetComponentInChildren(il2cpp_utils::GetSystemType("TMPro", "TextMeshProUGUI"), true));
                textObject->set_text(il2cpp_utils::createcsstr(chatObject->Text));
            }
        }
        while (maxVisibleObjects < chatObjects.size()) {
            ChatObject* chatObject = chatObjects.front();
            if (chatObject->GameObject)
                UnityEngine::Object::Destroy(chatObject->GameObject);
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
    client = new TwitchIRCClient();
    if (client->InitSocket()) {
        if (client->Connect()) {
            if (client->Login("justinfan" + std::to_string(1030307 + rand() % 1030307), "xxx")) {
                getLogger().info("Twitch Chat: Logged In!");
                client->HookIRCCommand("PRIVMSG", OnChatMessage);
                if (client->JoinChannel(Config.Channel)) {
                    getLogger().info("Twitch Chat: Joined Channel %s!", Config.Channel.c_str());
                    AddChatObject("<color=#55FF00FF>Joined Channel:</color> <color=#FFCC00FF>" + Config.Channel + "</color>");
                    while (client->Connected()) {
                        client->ReceiveData();
                    }
                }
            }
            AddChatObject("<color=#FF0000FF>Disconnected!</color>");
            client->Disconnect();
            getLogger().info("Twitch Chat: Disconnected!");
        }
    }
}
void FindChatObject_Template(){
    Array<UnityEngine::Component*>* children = customUIObject->GetComponentsInChildren(il2cpp_utils::GetSystemType("UnityEngine", "RectTransform"), true);
    for (int i = 0; i < children->Length(); i++) {
        UnityEngine::Component* object = children->values[i];
        if (object) {
            if ("ChatObject_Template" == to_utf8(csstrtostr(CRASH_UNLESS(object->get_name())))) {
                chatObject_Template = object->get_gameObject();
                chatObject_Template->SetActive(false);
            }
        } 
    }
}

void OnLoadAssetComplete(bs_utils::Asset* asset) {
    getLogger().info("Loaded Asset");
    customUIObject = CRASH_UNLESS(UnityEngine::Object::Instantiate((UnityEngine::GameObject*)asset));
    customUIObject->set_name(il2cpp_utils::createcsstr("ChatUI"));
    UnityEngine::Transform* objectTransform = CRASH_UNLESS(customUIObject->get_transform());
    objectTransform->set_position(isInMenu ? Config.PositionMenu : Config.PositionGame);
    objectTransform->set_eulerAngles(isInMenu ? Config.RotationMenu : Config.RotationGame);
    UnityEngine::Vector3 scale = isInMenu ? Config.ScaleMenu : Config.ScaleGame;
    scale.x *= 0.0025f;
    scale.y *= 0.0025f;
    scale.z *= 0.0025f;
    objectTransform->set_localScale(scale);
    il2cpp_utils::SetPropertyValue(objectTransform, "localScale", scale);
    customUIObject->SetActive(true);
    FindChatObject_Template();
    needUpdate = true;
    if (!threadStarted) {
        threadStarted = true;
        std::thread (TwitchIRCThread).detach();
    }
}

MAKE_HOOK_OFFSETLESS(Camera_FireOnPostRender, void, Il2CppObject* cam) {
    UpdateList();
    Camera_FireOnPostRender(cam);
}

MAKE_HOOK_OFFSETLESS(SceneManager_SetActiveScene, bool, UnityEngine::SceneManagement::Scene scene) {
    if (customUIObject) {
        if(isInMenu)
            customUIMenuObject = customUIObject; 
        chatObject_Template = nullptr;
        for (int i = 0; i < chatObjects.size(); i++) {
            UnityEngine::Object::Destroy(chatObjects[i]->GameObject);
            chatObjects[i]->GameObject = nullptr;
        }
        if(!isInMenu)
            UnityEngine::Object::Destroy(customUIObject);
        customUIObject = nullptr;
    }
    std::string name = to_utf8(csstrtostr(scene.get_name()));
    getLogger().info("Scene: %s", name.c_str());
    isInMenu = name == "MenuViewControllers";
    if (isInMenu || name == "GameCore") {
        const char* assetBundlePath = "/sdcard/Android/data/com.beatgames.beatsaber/files/uis/chatUI.qui";
        if (!assetBundle) {
            getLogger().info("Loading AssetBundle from chatUI.qui");
            bs_utils::AssetBundle::LoadFromFileAsync(assetBundlePath, [](bs_utils::AssetBundle* bundle){ 
                assetBundle = bundle;
                if (assetBundle) {
                    getLogger().info("Loading Asset from AssetBundle");
                    assetBundle->LoadAssetAsync("_customasset", [](bs_utils::Asset* asset){
                        OnLoadAssetComplete(asset);
                    }, il2cpp_utils::GetSystemType("UnityEngine", "GameObject"));
                }
            });
        } else {
            if(isInMenu){
                customUIObject = customUIMenuObject; 
                FindChatObject_Template();
                needUpdate = true;
            }else{
                getLogger().info("Loading Asset from AssetBundle");
                assetBundle->LoadAssetAsync("_customasset", [](bs_utils::Asset* asset){
                    OnLoadAssetComplete(asset);
                }, il2cpp_utils::GetSystemType("UnityEngine", "GameObject"));
            }
            
        }
    }
    return SceneManager_SetActiveScene(scene);
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
    AddChildVector(menuValue, "Scale", Config.ScaleMenu, allocator);
    configDoc.AddMember("Menu", menuValue, allocator);

    ConfigValue gameValue(rapidjson::kObjectType);
    AddChildVector(gameValue, "Position", Config.PositionGame, allocator);
    AddChildVector(gameValue, "Rotation", Config.RotationGame, allocator);
    AddChildVector(gameValue, "Scale", Config.ScaleGame, allocator);
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
        if (!ParseVector(Config.ScaleMenu, menuValue, "Scale")) foundEverything = false;
    } else foundEverything = false;

    if (configDoc.HasMember("Game") && configDoc["Game"].IsObject()) {
        ConfigValue gameValue = configDoc["Game"].GetObject();
        if (!ParseVector(Config.PositionGame, gameValue, "Position")) foundEverything = false;
        if (!ParseVector(Config.RotationGame, gameValue, "Rotation")) foundEverything = false;
        if (!ParseVector(Config.ScaleGame, gameValue, "Scale")) foundEverything = false;
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
    modInfo.version = "0.1.3";
    info = modInfo;
}

extern "C" void load() {
    il2cpp_functions::Init();
    if (!LoadConfig()) SaveConfig();
    getLogger().info("Starting ChatUI installation...");
    INSTALL_HOOK_OFFSETLESS(Camera_FireOnPostRender, il2cpp_utils::FindMethodUnsafe("UnityEngine", "Camera", "FireOnPostRender", 1));
    INSTALL_HOOK_OFFSETLESS(SceneManager_SetActiveScene, il2cpp_utils::FindMethodUnsafe("UnityEngine.SceneManagement", "SceneManager", "SetActiveScene", 1));
    getLogger().info("Successfully installed ChatUI!");
}