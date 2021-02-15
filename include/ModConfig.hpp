#pragma once
#include "config-utils/shared/config-utils.hpp"

DECLARE_CONFIG(ModConfig,

    DECLARE_VALUE(Channel, std::string, "Channel Name", "darknight1050");

    DECLARE_VALUE(PositionMenu, UnityEngine::Vector3, "Menu Position", UnityEngine::Vector3(2.0f, 3.6f, 3.5f));
    DECLARE_VALUE(RotationMenu, UnityEngine::Vector3, "Menu Rotation", UnityEngine::Vector3(-36.0f, 34.0f, 0.0f));
    DECLARE_VALUE(SizeMenu, UnityEngine::Vector2, "Menu Size", UnityEngine::Vector2(58.0f, 72.0f));

    DECLARE_VALUE(ForceGame, bool, "Force Game Values in Menu", false);

    DECLARE_VALUE(PositionGame, UnityEngine::Vector3, "Game Position", UnityEngine::Vector3(0.0f, 4.4f, 4.0f));
    DECLARE_VALUE(RotationGame, UnityEngine::Vector3, "Game Rotation", UnityEngine::Vector3(-42.0f, 0.0f, 0.0f));
    DECLARE_VALUE(SizeGame, UnityEngine::Vector2, "Game Size", UnityEngine::Vector2(58.0f, 88.0f));

    INIT_FUNCTION(
        INIT_VALUE(Channel);

        INIT_VALUE(PositionMenu);
        INIT_VALUE(RotationMenu);
        INIT_VALUE(SizeMenu);

        INIT_VALUE(ForceGame);
        
        INIT_VALUE(PositionGame);
        INIT_VALUE(RotationGame);
        INIT_VALUE(SizeGame);
    )
)