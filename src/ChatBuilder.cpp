#include "questui/shared/QuestUI.hpp"
#include "questui/shared/BeatSaberUI.hpp"
#include "questui/shared/CustomTypes/Components/Backgroundable.hpp"

#include "UnityEngine/RectOffset.hpp"
#include "UnityEngine/RectTransform.hpp"
#include "UnityEngine/UI/RectMask2D.hpp"
#include "HMUI/CurvedCanvasSettings.hpp"
#include "TMPro/TextMeshProUGUI.hpp"
#include "VRUIControls/VRGraphicRaycaster.hpp"

#include "ChatBuilder.hpp"
#include "customlogger.hpp"

using namespace QuestUI;
using namespace UnityEngine;
using namespace UnityEngine::UI;
using namespace HMUI;
using namespace TMPro;

//BAD STUFF I KNOW
UnityEngine::GameObject* chatGameObject = nullptr;
ChatUI::ChatHandler* chatHandler = nullptr;

void CreateChatGameObject() {
    if(chatGameObject) 
        return;
    chatGameObject = BeatSaberUI::CreateCanvas();
    Object::DestroyImmediate(chatGameObject->GetComponent<VRUIControls::VRGraphicRaycaster*>());
    Object::DontDestroyOnLoad(chatGameObject);
    
    chatHandler = chatGameObject->AddComponent<ChatUI::ChatHandler*>();
    chatGameObject->AddComponent<RectMask2D*>();
    chatGameObject->AddComponent<Backgroundable*>()->ApplyBackgroundWithAlpha(il2cpp_utils::createcsstr("round-rect-panel"), 0.8f);
    RectTransform* transform = chatGameObject->GetComponent<RectTransform*>();

    VerticalLayoutGroup* layout = BeatSaberUI::CreateVerticalLayoutGroup(transform);
    ContentSizeFitter* contentSizeFitter = layout->GetComponent<ContentSizeFitter*>();
    contentSizeFitter->set_horizontalFit(ContentSizeFitter::FitMode::Unconstrained);
    contentSizeFitter->set_verticalFit(ContentSizeFitter::FitMode::PreferredSize);
    layout->set_childControlWidth(false);
    layout->set_childControlHeight(true);
    layout->set_childForceExpandWidth(true);
    layout->set_childForceExpandHeight(false);
    layout->set_childAlignment(TextAnchor::LowerLeft);
    GameObject* layoutGameObject = layout->get_gameObject();
    RectTransform* layoutTransform = layout->get_rectTransform();
    layoutTransform->set_pivot(UnityEngine::Vector2(0.0f, 0.0f));
    chatHandler->LayoutTransform = layoutTransform;
    chatHandler->Canvas = chatGameObject;
}