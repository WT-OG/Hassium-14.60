#pragma once

namespace Enums
{
	enum ENetMode // https://docs.unrealengine.com/4.26/en-US/API/Runtime/Engine/Engine/ENetMode/ Documentation of All Enums!
	{
		NM_Standalone,
		NM_DedicatedServer,
		NM_ListenServer,
		NM_Client,
		NM_MAX,
	};
}

namespace Hooking {
	int Patch()
	{
		return 0;
	}
	bool Patch2()
	{
		return true;
	}
	char Patch3()
	{
		return 1;
	}
	bool Patch4()
	{
		return false;
	}

	Enums::ENetMode GetNetMode() { // https://docs.unrealengine.com/4.26/en-US/API/Runtime/Engine/Engine/UWorld/GetNetMode/ UWorldGetNetMode
		return Enums::ENetMode::NM_DedicatedServer;
	}

	Enums::ENetMode GetNetModeActor() // https://docs.unrealengine.com/4.26/en-US/API/Runtime/Engine/GameFramework/AActor/GetNetMode/
	{
		return Enums::ENetMode::NM_DedicatedServer;
	}

	static __int64 (*DispatchRequestOriginal)(__int64 a1, __int64* a2, int a3);

	static __int64 DispatchRequestHook(__int64 a1, __int64* a2, int a3)
	{
		return DispatchRequestOriginal(a1, a2, 3);
	}

	void (*TickFlushOriginal)(UNetDriver* NetDriver, float DeltaSeconds);
	void TickFlushHook(UNetDriver* NetDriver, float DeltaSeconds) // https://docs.unrealengine.com/4.26/en-US/API/Runtime/Engine/Engine/UNetDriver/TickFlush/
	{
		if (NetDriver->ReplicationDriver)
		{
			// ServerReplicateActors https://docs.unrealengine.com/4.26/en-US/API/Runtime/Engine/Engine/UNetDriver/ServerReplicateActors/
			
			Listening::ServerReplicateActors(NetDriver->ReplicationDriver);
		}
		return TickFlushOriginal(NetDriver, DeltaSeconds);
	}

	bool(*ReadyToStartMatchOriginal)(AFortGameModeAthena* GameMode);
	bool ReadyToStartMatchHook(AFortGameModeAthena* GameMode)
	{
		TArray<AActor*> WarmupActors;
		UGameplayStatics::GetDefaultObj()->GetAllActorsOfClass(UWorld::GetWorld(), AFortPlayerStartWarmup::StaticClass(), &WarmupActors);

		int WarmupSpots = WarmupActors.Num();

		WarmupActors.Free();

		if (WarmupSpots == 0)
			return false;

		static bool bReadyToStartMatchHook = false;
		if (bReadyToStartMatchHook)
			return false;
		bReadyToStartMatchHook = true;
		static bool bInit = false;
		if (!bInit)
		{
			bInit = true;
			UFortPlaylistAthena* Playlist = StaticFindObject<UFortPlaylistAthena>("/Game/Athena/Playlists/Playlist_DefaultSolo.Playlist_DefaultSolo");
			if (!Playlist)
			{
				return false;
			}
			GetGameMode()->WarmupRequiredPlayerCount = 1;
			GetGameState()->CurrentPlaylistInfo.BasePlaylist = Playlist;
			GetGameState()->CurrentPlaylistInfo.OverridePlaylist = Playlist;
			GetGameState()->CurrentPlaylistInfo.PlaylistReplicationKey++;
			GetGameState()->CurrentPlaylistInfo.MarkArrayDirty();
			GetGameState()->OnRep_CurrentPlaylistInfo();

			GetGameState()->CurrentPlaylistId = Playlist->PlaylistId;
			GetGameState()->OnRep_CurrentPlaylistId();

			GetGameMode()->CurrentPlaylistName = Playlist->PlaylistName;
			GetGameMode()->CurrentPlaylistId = Playlist->PlaylistId;
			
			static bool Listening = false;
			if (!Listening)
			{
				Listening = true;
				Listening::Listen();
				
				auto TimeSeconds = UGameplayStatics::GetDefaultObj()->GetTimeSeconds(UWorld::GetWorld());
				auto DR = 60.0f;
				GetGameState()->WarmupCountdownEndTime = TimeSeconds + DR;
				GetGameMode()->WarmupCountdownDuration = DR;
				GetGameState()->WarmupCountdownStartTime = TimeSeconds;
				GetGameMode()->WarmupEarlyCountdownDuration = DR;
				GetGameMode()->bWorldIsReady = true;
			}

		}

		bool Ret = GameMode->AlivePlayers.Num() > 0;
		if (!Ret)
		{
			auto TimeSeconds = UGameplayStatics::GetDefaultObj()->GetTimeSeconds(UWorld::GetWorld());
			auto DR = 60.0f;
			GetGameState()->WarmupCountdownEndTime = TimeSeconds + DR;
			GetGameMode()->WarmupCountdownDuration = DR;
			GetGameState()->WarmupCountdownStartTime = TimeSeconds;
			GetGameMode()->WarmupEarlyCountdownDuration = DR;
		}

		return Ret;
	}
	static uint8 NextTeam = 3; //Dont change or bad sigma
	static uint8 LastPlayers = 0;
	EFortTeam PickTeamHook(AFortGameModeAthena* GM, uint8 Preffered, AFortPlayerControllerAthena* PC)
	{
		uint8 Return = 3;
		AFortGameStateAthena* GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;
		LastPlayers++;
		if (LastPlayers >= GameState->CurrentPlaylistInfo.BasePlaylist->MaxSquadSize)
		{
			NextTeam++;
			LastPlayers = 0;
		}

		return EFortTeam(Return);
	}
	

	void (*HandleStartingNewPlayerOG)(AFortGameModeAthena* GM, AFortPlayerControllerAthena* PC);
	void HandleStartingNewPlayerHook(AFortGameModeAthena* GM, AFortPlayerControllerAthena* PC)
	{
		FGameMemberInfo PlayerInfo;
		PlayerInfo.MostRecentArrayReplicationKey = -1;
		PlayerInfo.ReplicationID = -1;
		PlayerInfo.ReplicationKey = -1;
		PlayerInfo.SquadId = ((AFortPlayerStateAthena*)PC->PlayerState)->SquadId;
		PlayerInfo.TeamIndex = ((AFortPlayerStateAthena*)PC->PlayerState)->TeamIndex;
		PlayerInfo.MemberUniqueId = PC->PlayerState->UniqueId;

		GetGameState()->GameMemberInfoArray.Members.Add(PlayerInfo);
		GetGameState()->GameMemberInfoArray.MarkArrayDirty();

		return HandleStartingNewPlayerOG(GM, PC);
	}

	APawn* SpawnDefaultPawnFor(AGameModeBase* GameMode, AController* NewPlayer, AActor* StartSpot)
	{
		if (NewPlayer && StartSpot)
		{
			auto Transform = StartSpot->GetTransform();

			return GameMode->SpawnDefaultPawnAtTransform(NewPlayer, Transform);
		}

		return 0;
	}
}
