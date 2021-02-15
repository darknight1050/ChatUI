#pragma once

#include <vector>
#include <mutex>

#include "UnityEngine/MonoBehaviour.hpp"
#include "UnityEngine/RectTransform.hpp"

#include "custom-types/shared/macros.hpp"

#include "ChatObject.hpp"

DECLARE_CLASS_CODEGEN(ChatUI, ChatHandler, UnityEngine::MonoBehaviour,

    private:
        std::vector<ChatObject> chatObjects;
        std::mutex chatObjectsMutex;

    public:
        void AddChatObject(ChatObject object);
        void SetPosition(UnityEngine::Vector3 position);
        void SetRotation(UnityEngine::Vector3 rotation);
        void SetSize(UnityEngine::Vector2 size);
    
    DECLARE_INSTANCE_FIELD(UnityEngine::GameObject*, Canvas);
    DECLARE_INSTANCE_FIELD(UnityEngine::RectTransform*, LayoutTransform);

    DECLARE_METHOD(void, Update);
    
    REGISTER_FUNCTION(Backgroundable,
        REGISTER_FIELD(LayoutTransform);
        REGISTER_METHOD(Update);
    )
    
)