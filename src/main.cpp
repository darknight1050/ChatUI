#include "../include/main.hpp"

static rapidjson::Document& config_doc = Configuration::config;

static TwitchIRCClient* client = nullptr;
static std::map<std::string, std::string> usersColorCache;

static Il2CppObject* assetBundle = nullptr;
static Il2CppObject* customUIObject = nullptr;
static Il2CppObject* chatObject_Template = nullptr;
static std::vector<ChatObject*> chatObjects;
static std::vector<ChatObject*> chatObjectsToAdd;
static const int maxVisibleObjects = 24;

static long long lastUpdate = 0;
static bool needUpdate = false;
static bool threadStarted = false;
static bool isInMenu = false;

static bool isLoadingAsset = false;
static bool reloadAsset = false;

void UpdateList(){
    if(!chatObjectsToAdd.empty())
        needUpdate = true;
    long long currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();;
    if(chatObject_Template != nullptr && needUpdate && currentTime-lastUpdate > 100){
        needUpdate = false;
        lastUpdate = currentTime;
        if(!chatObjectsToAdd.empty()){
            chatObjects.push_back(chatObjectsToAdd[0]);
            chatObjectsToAdd.erase(chatObjectsToAdd.begin());
        }
        for(int i = 0;i<chatObjects.size();i++){
                ChatObject* chatObject = chatObjects[i];
                if(chatObject->GameObject == nullptr){
                    ChatObject* lastChatObject = nullptr;
                    if(i > 0)
                        lastChatObject = chatObjects[i-1];
                   // Vector3 lastChatObjectPosition;
                   // Vector3 lastChatObjectSize;
                    if(lastChatObject != nullptr && lastChatObject->GameObject != nullptr){
                        Vector3 lastChatObjectPosition =  *CRASH_UNLESS(il2cpp_utils::RunMethod<Vector3>(lastChatObject->GameObject, "get_localPosition"));
                        Vector3 lastChatObjectSize = *CRASH_UNLESS(il2cpp_utils::RunMethod<Vector3>(lastChatObject->GameObject, "get_sizeDelta"));
                    }
                    chatObject->GameObject = *CRASH_UNLESS(il2cpp_utils::RunMethod<Il2CppObject*>(il2cpp_utils::GetClassFromName("UnityEngine", "Object"), "Instantiate", chatObject_Template));
                    il2cpp_utils::RunMethod(chatObject->GameObject, "set_name", il2cpp_utils::createcsstr("ChatObject"));
                    UnityHelper::SetSameParent(chatObject->GameObject, chatObject_Template);
                    UnityHelper::SetGameObjectActive(chatObject->GameObject, true);
                    UnityHelper::SetButtonText(chatObject->GameObject, chatObject->Text);
                }
            }
        while(maxVisibleObjects<chatObjects.size()){
            ChatObject* chatObject = chatObjects[0];
            if(chatObject->GameObject != nullptr)
               il2cpp_utils::RunMethod(il2cpp_utils::GetClassFromName("UnityEngine", "Object"), "Destroy", UnityHelper::GetGameObject(chatObject->GameObject));
            delete chatObject;
            chatObjects.erase(chatObjects.begin());
        }
    }
}

void AddChatObject(std::string text){
    ChatObject* chatObject = new ChatObject();
    chatObject->Text.assign(text.c_str(), text.size());
    chatObjectsToAdd.push_back(chatObject);   
}

void OnChatMessage(IRCMessage message, TwitchIRCClient* client)
{
    for(std::string name : *Config.Blacklist){
        if(name.compare(message.prefix.nick) == 0){
            log(INFO, "Twitch Chat: Blacklisted user %s sent the message: %s", message.prefix.nick.c_str(), message.parameters.at(message.parameters.size() - 1).c_str());
            return;
        }
    }
    if(usersColorCache.find(message.prefix.nick) == usersColorCache.end())
        usersColorCache.insert(pair<std::string, std::string>(message.prefix.nick, int_to_hex(rand() % 0x1000000, 6)));
    std::string text = "<color=" + usersColorCache[message.prefix.nick] + ">" + message.prefix.nick + "</color>: " + message.parameters.at(message.parameters.size() - 1);
    log(INFO, "Twitch Chat: %s", text.c_str());
    AddChatObject(text);
}

void TwitchIRCThread(){
    client = new TwitchIRCClient();
    if (client->InitSocket())
    {
        if (client->Connect())
        {
            if (client->Login(Config.Nick, Config.OAuth))
            {
                log(INFO, "Twitch Chat: Logged In as %s!", Config.Nick.c_str());
                client->HookIRCCommand("PRIVMSG", OnChatMessage);
                if (client->JoinChannel(Config.Channel)){
                    log(INFO, "Twitch Chat: Joined Channel %s!", Config.Channel.c_str());
                    while (client->Connected())
                        client->ReceiveData();
                }
            }
            log(INFO, "Twitch Chat: Disconnected!");
        }
    }
}

void OnLoadAssetComplete(Il2CppObject* asset){
    customUIObject = *CRASH_UNLESS(il2cpp_utils::RunMethod<Il2CppObject*>(il2cpp_utils::GetClassFromName("UnityEngine", "Object"), "Instantiate", asset));

    Il2CppObject* objectTransform;

    objectTransform = *CRASH_UNLESS(il2cpp_utils::RunMethod<Il2CppObject*>(customUIObject, "get_transform"));
    Vector3 scale;
    if(isInMenu){
        il2cpp_utils::RunMethod(objectTransform, "set_position", Config.PositionMenu);
        il2cpp_utils::RunMethod(objectTransform, "set_eulerAngles", Config.RotationMenu);
        scale = Config.ScaleMenu;
    }else{
        il2cpp_utils::RunMethod(objectTransform, "set_position", Config.PositionGame);
        il2cpp_utils::RunMethod(objectTransform, "set_eulerAngles", Config.RotationGame);
        scale = Config.ScaleGame;
    }
    scale.x *= 0.0025f;
    scale.y *= 0.0025f;
    scale.z *= 0.0025f;
    il2cpp_utils::RunMethod(objectTransform, "set_localScale", scale);

    UnityHelper::SetGameObjectActive(customUIObject, true);
    chatObject_Template = UnityHelper::GetComponentInChildren(customUIObject, il2cpp_utils::GetClassFromName("UnityEngine", "RectTransform"), "ChatObject_Template");
    UnityHelper::SetGameObjectActive(chatObject_Template, false);
    needUpdate = true;
    if(!threadStarted){
        threadStarted = true;
        std::thread twitchIRCThread(TwitchIRCThread);
        twitchIRCThread.detach();
    }
}

void OnLoadAssetBundleComplete(Il2CppObject* assetBundleArg){
    assetBundle = assetBundleArg;
    UnityAssetLoader::LoadAssetFromAssetBundleAsync(assetBundle, (UnityAssetLoader_OnLoadAssetCompleteFunction*)OnLoadAssetComplete);
}

MAKE_HOOK_OFFSETLESS(Camera_FireOnPostRender, void, Il2CppObject* cam)
{   
    Camera_FireOnPostRender(cam);
    UpdateList();
}
static AssetImporter* importer;
MAKE_HOOK_OFFSETLESS(SceneManager_SetActiveScene, bool, int scene)
{
    if(customUIObject != nullptr){
        chatObject_Template = nullptr;
        for(int i = 0; i<chatObjects.size(); i++){
            chatObjects[i]->GameObject = nullptr;
        }
        il2cpp_utils::RunMethod(il2cpp_utils::GetClassFromName("UnityEngine", "Object"), "Destroy", customUIObject);
        customUIObject = nullptr;
        log(INFO, "Destroyed ChatUI!");
    }
    Il2CppString* nameObject;
    nameObject = *CRASH_UNLESS(il2cpp_utils::RunMethod<Il2CppString*>(il2cpp_utils::GetClassFromName("UnityEngine.SceneManagement", "Scene"), "GetNameInternal", scene));
    const char* name = to_utf8(csstrtostr(nameObject)).c_str();
    log(INFO, "Scene: %s", name);
    isInMenu = strcmp(name, "MenuViewControllers") == 0;
    if(isInMenu || strcmp(name, "GameCore") == 0) {
        //if(assetBundle == nullptr){
            log(INFO, "AssetImporter: Loading from chatUI.qui");
            
        if (!importer) {
            log(DEBUG, "Making importer");
            importer = new AssetImporter("/sdcard/Android/data/com.beatgames.beatsaber/files/uis/chatUI.qui", "_customasset");
            importer->LoadAssetBundle((UnityAssetLoader_OnLoadAssetCompleteFunction*)OnLoadAssetComplete,true);
        }
            
           // UnityAssetLoader::LoadAssetBundleFromFileAsync("/sdcard/Android/data/com.beatgames.beatsaber/files/uis/chatUI.qui", (UnityAssetLoader_OnLoadAssetBundleCompleteFunction*)OnLoadAssetBundleComplete);
        else {
            importer->LoadAsset((UnityAssetLoader_OnLoadAssetCompleteFunction*)OnLoadAssetComplete,"_customasset");
            //UnityAssetLoader::LoadAssetFromAssetBundleAsync(assetBundle, (UnityAssetLoader_OnLoadAssetCompleteFunction*)OnLoadAssetComplete);
        }
    }
    return SceneManager_SetActiveScene(scene);
}

void SaveConfig() {
    log(INFO, "Saving Configuration...");
    config_doc.RemoveAllMembers();
    config_doc.SetObject();
    rapidjson::Document::AllocatorType& allocator = config_doc.GetAllocator();
    config_doc.AddMember("Nick", std::string(Config.Nick), allocator);
    config_doc.AddMember("OAuth", std::string(Config.OAuth), allocator);
    config_doc.AddMember("Channel", std::string(Config.Channel), allocator);
    rapidjson::Value menuValue(rapidjson::kObjectType);
    {
        rapidjson::Value positionValue(rapidjson::kObjectType);
        positionValue.AddMember("X", Config.PositionMenu.x, allocator);
        positionValue.AddMember("Y", Config.PositionMenu.y, allocator);
        positionValue.AddMember("Z", Config.PositionMenu.z, allocator);
        menuValue.AddMember("Position", positionValue, allocator);
        rapidjson::Value rotationValue(rapidjson::kObjectType);
        rotationValue.AddMember("X", Config.RotationMenu.x, allocator);
        rotationValue.AddMember("Y", Config.RotationMenu.y, allocator);
        rotationValue.AddMember("Z", Config.RotationMenu.z, allocator);
        menuValue.AddMember("Rotation", rotationValue, allocator);
        rapidjson::Value scaleValue(rapidjson::kObjectType);
        scaleValue.AddMember("X", Config.ScaleMenu.x, allocator);
        scaleValue.AddMember("Y", Config.ScaleMenu.y, allocator);
        scaleValue.AddMember("Z", Config.ScaleMenu.z, allocator);
        menuValue.AddMember("Scale", scaleValue, allocator);
        config_doc.AddMember("Menu", menuValue, allocator);
    }
    rapidjson::Value gameValue(rapidjson::kObjectType);
    {
        rapidjson::Value positionValue(rapidjson::kObjectType);
        positionValue.AddMember("X", Config.PositionGame.x, allocator);
        positionValue.AddMember("Y", Config.PositionGame.y, allocator);
        positionValue.AddMember("Z", Config.PositionGame.z, allocator);
        gameValue.AddMember("Position", positionValue, allocator);
        rapidjson::Value rotationValue(rapidjson::kObjectType);
        rotationValue.AddMember("X", Config.RotationGame.x, allocator);
        rotationValue.AddMember("Y", Config.RotationGame.y, allocator);
        rotationValue.AddMember("Z", Config.RotationGame.z, allocator);
        gameValue.AddMember("Rotation", rotationValue, allocator);
        rapidjson::Value scaleValue(rapidjson::kObjectType);
        scaleValue.AddMember("X", Config.ScaleGame.x, allocator);
        scaleValue.AddMember("Y", Config.ScaleGame.y, allocator);
        scaleValue.AddMember("Z", Config.ScaleGame.z, allocator);
        gameValue.AddMember("Scale", scaleValue, allocator);
        config_doc.AddMember("Game", gameValue, allocator);
    }
    rapidjson::Value Blacklist(rapidjson::kArrayType);
    {
        for(std::string name : *Config.Blacklist){
            rapidjson::Value nameValue;
            nameValue.SetString(name.c_str(), name.length(), allocator);
            Blacklist.PushBack(nameValue, allocator);
        }
        config_doc.AddMember("Blacklist", Blacklist, allocator);
    }
    Configuration::Write();
    log(INFO, "Saved Configuration!");
}

bool LoadConfig() { 
    log(INFO, "Loading Configuration...");
    Configuration::Load();
    bool foundEverything = true;
    if(config_doc.HasMember("Nick") && config_doc["Nick"].IsString()){
        char* buffer = (char*)malloc(config_doc["Nick"].GetStringLength());
        std::string data(config_doc["Nick"].GetString());
        transform(data.begin(), data.end(), data.begin(), [](unsigned char c){ return tolower(c); });
        Config.Nick = data;
    }else{
        foundEverything = false;
    }
    if(config_doc.HasMember("OAuth") && config_doc["OAuth"].IsString()){
        char* buffer = (char*)malloc(config_doc["OAuth"].GetStringLength());
        strcpy(buffer, config_doc["OAuth"].GetString());
        Config.OAuth = std::string(buffer);   
        if(Config.OAuth.rfind("oauth:", 0) != 0)
            Config.OAuth = "oauth:" + Config.OAuth;
    }else{
        foundEverything = false;
    }
    if(config_doc.HasMember("Channel") && config_doc["Channel"].IsString()){
        char* buffer = (char*)malloc(config_doc["Channel"].GetStringLength());
        std::string data(config_doc["Channel"].GetString());
        transform(data.begin(), data.end(), data.begin(), [](unsigned char c){ return tolower(c); });
        strcpy(buffer, data.c_str());
        Config.Channel = data;
    }else{
        foundEverything = false;
    }
    if(config_doc.HasMember("Menu") && config_doc["Menu"].IsObject()){
        rapidjson::Value menuValue = config_doc["Menu"].GetObject();
        if(menuValue.HasMember("Position") && menuValue["Position"].IsObject()){
            rapidjson::Value positionValue = menuValue["Position"].GetObject();
            if(positionValue.HasMember("X") && positionValue["X"].IsFloat() && positionValue.HasMember("Y") && positionValue["Y"].IsFloat() && positionValue.HasMember("Z") && positionValue["Z"].IsFloat()){
                Config.PositionMenu.x = positionValue["X"].GetFloat();
                Config.PositionMenu.y = positionValue["Y"].GetFloat();
                Config.PositionMenu.z = positionValue["Z"].GetFloat();
            }else{
                foundEverything = false;
            }
        }else{
            foundEverything = false;
        }
        if(menuValue.HasMember("Rotation") && menuValue["Rotation"].IsObject()){
            rapidjson::Value rotationValue = menuValue["Rotation"].GetObject();
            if(rotationValue.HasMember("X") && rotationValue["X"].IsFloat() && rotationValue.HasMember("Y") && rotationValue["Y"].IsFloat() && rotationValue.HasMember("Z") && rotationValue["Z"].IsFloat()){
                Config.RotationMenu.x = rotationValue["X"].GetFloat();
                Config.RotationMenu.y = rotationValue["Y"].GetFloat();
                Config.RotationMenu.z = rotationValue["Z"].GetFloat();
            }else{
                foundEverything = false;
            }
        }else{
            foundEverything = false;
        }
        if(menuValue.HasMember("Scale") && menuValue["Scale"].IsObject()){
            rapidjson::Value scaleValue = menuValue["Scale"].GetObject();
            if(scaleValue.HasMember("X") && scaleValue["X"].IsFloat() && scaleValue.HasMember("Y") && scaleValue["Y"].IsFloat() && scaleValue.HasMember("Z") && scaleValue["Z"].IsFloat()){
                Config.ScaleMenu.x = scaleValue["X"].GetFloat();
                Config.ScaleMenu.y = scaleValue["Y"].GetFloat();
                Config.ScaleMenu.z = scaleValue["Z"].GetFloat();
            }else{
                foundEverything = false;
            }
        }else{
            foundEverything = false;
        }
    }else{
        foundEverything = false;
    }
    if(config_doc.HasMember("Game") && config_doc["Game"].IsObject()){
        rapidjson::Value gameValue = config_doc["Game"].GetObject();
        if(gameValue.HasMember("Position") && gameValue["Position"].IsObject()){
            rapidjson::Value positionValue = gameValue["Position"].GetObject();
            if(positionValue.HasMember("X") && positionValue["X"].IsFloat() && positionValue.HasMember("Y") && positionValue["Y"].IsFloat() && positionValue.HasMember("Z") && positionValue["Z"].IsFloat()){
                Config.PositionGame.x = positionValue["X"].GetFloat();
                Config.PositionGame.y = positionValue["Y"].GetFloat();
                Config.PositionGame.z = positionValue["Z"].GetFloat();
            }else{
                foundEverything = false;
            }
        }else{
            foundEverything = false;
        }
        if(gameValue.HasMember("Rotation") && gameValue["Rotation"].IsObject()){
            rapidjson::Value rotationValue = gameValue["Rotation"].GetObject();
            if(rotationValue.HasMember("X") && rotationValue["X"].IsFloat() && rotationValue.HasMember("Y") && rotationValue["Y"].IsFloat() && rotationValue.HasMember("Z") && rotationValue["Z"].IsFloat()){
                Config.RotationGame.x = rotationValue["X"].GetFloat();
                Config.RotationGame.y = rotationValue["Y"].GetFloat();
                Config.RotationGame.z = rotationValue["Z"].GetFloat();
            }else{
                foundEverything = false;
            }
        }else{
            foundEverything = false;
        }
        if(gameValue.HasMember("Scale") && gameValue["Scale"].IsObject()){
            rapidjson::Value scaleValue = gameValue["Scale"].GetObject();
            if(scaleValue.HasMember("X") && scaleValue["X"].IsFloat() && scaleValue.HasMember("Y") && scaleValue["Y"].IsFloat() && scaleValue.HasMember("Z") && scaleValue["Z"].IsFloat()){
                Config.ScaleGame.x = scaleValue["X"].GetFloat();
                Config.ScaleGame.y = scaleValue["Y"].GetFloat();
                Config.ScaleGame.z = scaleValue["Z"].GetFloat();
            }else{
                foundEverything = false;
            }
        }else{
            foundEverything = false;
        }
    }else{
        foundEverything = false;
    }
    if(Config.Blacklist == nullptr)
        Config.Blacklist = new std::vector<std::string>();
    for(std::string name : *Config.Blacklist){
        free((void*)name.c_str());
    }
    Config.Blacklist->clear();
    if(config_doc.HasMember("Blacklist") && config_doc["Blacklist"].IsArray()){
        for (rapidjson::SizeType i = 0; i < config_doc["Blacklist"].Size(); i++){
            if(config_doc["Blacklist"][i].IsString()){
                char* buffer = (char*)malloc(config_doc["Blacklist"][i].GetStringLength());
                strcpy(buffer, config_doc["Blacklist"][i].GetString());
                Config.Blacklist->push_back(std::string(buffer));
            }
        }
    }else{
        Config.Blacklist->push_back(std::string("dootybot"));
        Config.Blacklist->push_back(std::string("nightbot"));
        foundEverything = false;
    }
    if(foundEverything){
        log(INFO, "Loaded Configuration!");
        return true;
    }
    return false;
}

extern "C" void load()
{
    if(!LoadConfig())
        SaveConfig();
    log(INFO, "Starting ChatUI installation...");
    il2cpp_functions::Init();
    INSTALL_HOOK_OFFSETLESS(Camera_FireOnPostRender, il2cpp_utils::FindMethodUnsafe("UnityEngine", "Camera", "FireOnPostRender", 1));
    INSTALL_HOOK_OFFSETLESS(SceneManager_SetActiveScene, il2cpp_utils::FindMethodUnsafe("UnityEngine.SceneManagement", "SceneManager", "SetActiveScene", 1));
    
    log(INFO, "Successfully installed ChatUI!");
}