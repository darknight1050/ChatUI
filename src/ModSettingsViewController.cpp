#include "ModSettingsViewController.hpp"

#include "HMUI/Touchable.hpp"

#include "questui/shared/BeatSaberUI.hpp"

#include "customlogger.hpp"
#include "ModConfig.hpp"

using namespace QuestUI;
using namespace UnityEngine;
using namespace UnityEngine::UI;
using namespace HMUI;

void DidActivate(ViewController* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
    if(firstActivation) {
        self->get_gameObject()->AddComponent<Touchable*>();

        GameObject* container = BeatSaberUI::CreateScrollableSettingsContainer(self->get_transform());

        AddConfigValueStringSetting(container->get_transform(), getModConfig().Channel);

        AddConfigValueIncrementVector3(container->get_transform(), getModConfig().PositionMenu, 2, 0.05f);
        AddConfigValueIncrementVector3(container->get_transform(), getModConfig().RotationMenu, 0, 1.0f);
        AddConfigValueIncrementVector2(container->get_transform(), getModConfig().SizeMenu, 0, 1.0f);

        AddConfigValueToggle(container->get_transform(), getModConfig().ForceGame);

        AddConfigValueIncrementVector3(container->get_transform(), getModConfig().PositionGame, 2, 0.05f);
        AddConfigValueIncrementVector3(container->get_transform(), getModConfig().RotationGame, 0, 1.0f);
        AddConfigValueIncrementVector2(container->get_transform(), getModConfig().SizeGame, 0, 1.0f);
    }
}