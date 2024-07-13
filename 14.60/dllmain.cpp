#include "ue.h"
#include "memory.h"
#include "log.h"
#include "listen.h"
#include "hook.h"

DWORD Main(LPVOID)
{
    if (bDebugging)
    {
        AllocConsole();
        FILE* sFile;
        freopen_s(&sFile, "CONOUT$", "w", stdout);
        SetConsoleTitleA("Hassium 14.60 || Loading Funcs....");
    }
    MH_Initialize();
    InitGObjects();
    FFree = decltype(FFree)(Memory::MergeOffset(Offsets::Free));

    MH_CreateHook((LPVOID)Memory::MergeOffset(Offsets::GetNetMode), Hooking::GetNetMode, nullptr);
    MH_EnableHook((LPVOID)Memory::MergeOffset(Offsets::GetNetMode));

    MH_CreateHook((LPVOID)Memory::MergeOffset(Offsets::KickPlayer), Hooking::Patch4, nullptr);
    MH_EnableHook((LPVOID)Memory::MergeOffset(Offsets::KickPlayer));

    *(bool*)(Memory::MergeOffset(0x939a30d)) = false; //GIsClient, no need for actornetmode with gIsClient

    MH_CreateHook((LPVOID)Memory::MergeOffset(Offsets::CollectGarbage), Hooking::Patch4, nullptr);
    MH_EnableHook((LPVOID)Memory::MergeOffset(Offsets::CollectGarbage));


    UKismetSystemLibrary::GetDefaultObj()->ExecuteConsoleCommand(UWorld::GetWorld(), L"open Apollo_Terrain", nullptr);
    UFortEngine::GetEngine()->GameInstance->LocalPlayers.Remove(0); //Tbh not quite sure why I decided to remove the player before loading the level, I am very smart I swear.

    Memory::HookAddress(Memory::MergeOffset(0x2066c40), Hooking::PickTeamHook);

    MH_CreateHook((LPVOID)Memory::MergeOffset(Offsets::ReadyToStartMatch), Hooking::ReadyToStartMatchHook, (void**)&Hooking::ReadyToStartMatchOriginal);
    MH_EnableHook((LPVOID)Memory::MergeOffset(Offsets::ReadyToStartMatch));

    MH_CreateHook((LPVOID)Memory::MergeOffset(Offsets::SpawnDefaultPawnFor), Hooking::SpawnDefaultPawnFor, nullptr);
    MH_EnableHook((LPVOID)Memory::MergeOffset(Offsets::SpawnDefaultPawnFor));

    Memory::VirtualHook(AFortGameModeAthena::GetDefaultObj()->Vft, 0xCF, Hooking::HandleStartingNewPlayerHook, (void**)&Hooking::HandleStartingNewPlayerOG);

    MH_CreateHook((LPVOID)Memory::MergeOffset(Offsets::TickFlush), Hooking::TickFlushHook, (void**)&Hooking::TickFlushOriginal);
    MH_EnableHook((LPVOID)Memory::MergeOffset(Offsets::TickFlush));

    return 0;
}

BOOL APIENTRY DllMain( HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        CreateThread(0, 0, Main, 0, 0, 0);
        break;
    }
    return TRUE;
}

