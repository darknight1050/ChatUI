# Builds a .zip file for loading with BMBF
& $PSScriptRoot/build.ps1

if ($?) {
    Compress-Archive -Path "./libs/arm64-v8a/libChatUI_2.0.so", "./libs/arm64-v8a/libbeatsaber-hook_0_8_4.so", "./bmbfmod.json" -DestinationPath "./ChatUI_2.0_v0.1.0.zip" -Update
}
