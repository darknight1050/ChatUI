#pragma once
#include "UnityEngine/GameObject.hpp"
#include "CustomTypes/ChatHandler.hpp"

extern UnityEngine::GameObject* chatGameObject;
extern ChatUI::ChatHandler* chatHandler;

void CreateChatGameObject();