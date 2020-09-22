# Builds a .zip file for loading with BMBF
$NDKPath = Get-Content $PSScriptRoot/ndkpath.txt

$buildScript = "$NDKPath/build/ndk-build"
if (-not ($PSVersionTable.PSEdition -eq "Core")) {
    $buildScript += ".cmd"
}

& $buildScript NDK_PROJECT_PATH=$PSScriptRoot APP_BUILD_SCRIPT=$PSScriptRoot/Android.mk NDK_APPLICATION_MK=$PSScriptRoot/Application.mk
Compress-Archive -Force -Path "./libs/arm64-v8a/libchatui.so","./bmbfmod.json","./chatUI.qui","./extern/libbeatsaber-hook_0_6_0.so","./extern/libbs-utils.so","./extern/libcodegen_0_2_4.so" -DestinationPath "./chatui_v0.1.3.zip"
