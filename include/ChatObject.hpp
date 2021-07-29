#pragma once

#include <string>

#include "UnityEngine/GameObject.hpp"

struct ChatObject {
    std::string Text;
    UnityEngine::GameObject* GameObject;
};