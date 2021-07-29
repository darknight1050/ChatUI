& ./build.ps1
& adb push libs/arm64-v8a/libchatui.so /sdcard/Android/data/com.beatgames.beatsaber/files/mods/libchatui.so
& adb shell am force-stop com.beatgames.beatsaber
& adb shell am start com.beatgames.beatsaber/com.unity3d.player.UnityPlayerActivity