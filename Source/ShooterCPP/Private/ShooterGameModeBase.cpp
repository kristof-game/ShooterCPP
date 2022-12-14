// ShooterGame. All Rights Reserved.

#include "ShooterGameModeBase.h"
#include "Player/ShooterBaseCharacter.h"
#include "Player/ShooterPlayerController.h"
#include "UI/ShooterGameHUD.h"
#include "AIController.h"
#include "Player/ShooterPlayerState.h"
#include "ShooterUtils.h"
#include "Components/ShooterRespawnComponent.h"
#include "EngineUtils.h"
#include "ShooterGameInstance.h"
#include "Components/ShooterWeaponComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogShooterGameModeBase, All, All);

AShooterGameModeBase::AShooterGameModeBase()
{
	DefaultPawnClass = AShooterBaseCharacter::StaticClass();
	PlayerControllerClass = AShooterPlayerController::StaticClass();
	HUDClass = AShooterGameHUD::StaticClass();
	PlayerStateClass = AShooterPlayerState::StaticClass();
}

void AShooterGameModeBase::StartPlay()
{
	Super::StartPlay();

	SpawnBots();
	CreateTeamsInfo();
	CurrentRound = 1;
	StartRound();

	SetMatchState(EShooterMatchState::InProgress);
}

UClass* AShooterGameModeBase::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	if (InController && InController->IsA<AAIController>())
	{
		return AIPawnClass;
	}
	return Super::GetDefaultPawnClassForController_Implementation(InController);
}

void AShooterGameModeBase::Killed(AController* KillerController, AController* VictimController)
{
	const auto KillerPlayerState = KillerController ? Cast<AShooterPlayerState>(KillerController->PlayerState) : nullptr;
	const auto VictimPlayerState = VictimController ? Cast<AShooterPlayerState>(VictimController->PlayerState) : nullptr;

	StartRespawn(VictimController);

	if (!KillerPlayerState || !VictimPlayerState)
		return;

	if (KillerPlayerState == VictimPlayerState)
	{
		KillerPlayerState->ReduceKill();
		return;
	}

	KillerPlayerState->AddKill();
	VictimPlayerState->AddDeath();
}

void AShooterGameModeBase::RespawnRequest(AController* Controller)
{
	ResetOnePlayer(Controller);
}

bool AShooterGameModeBase::SetPause(APlayerController* PC, FCanUnpause CanUnpauseDelegate)
{
	const auto PauseSet = Super::SetPause(PC, CanUnpauseDelegate);

	if (PauseSet)
	{
		StopAllFire();
		SetMatchState(EShooterMatchState::Pause);
	}
	return PauseSet;
}

bool AShooterGameModeBase::ClearPause()
{
	const auto PauseCleared = Super::ClearPause();

	if (PauseCleared)
	{
		SetMatchState(EShooterMatchState::InProgress);
	}
	return PauseCleared;
}

void AShooterGameModeBase::SpawnBots()
{
	if (!GetWorld())
		return;

	for (int32 i = 0; i < GameData.PlayersNum - 1; i++)
	{
		FActorSpawnParameters SpawnInfo;
		SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		const auto ShooterAIController = GetWorld()->SpawnActor<AAIController>(AIControllerClass, SpawnInfo);
		RestartPlayer(ShooterAIController);
	}
}

void AShooterGameModeBase::StartRound()
{
	RoundCountDown = GameData.RoundTime;
	GetWorldTimerManager().SetTimer(GameRoundTimerHandle, this, &AShooterGameModeBase::GameTimerUpdate, 1.0f, true);
}

void AShooterGameModeBase::GameTimerUpdate()
{
	RoundCountDown--;
	if (RoundCountDown == 0)
	{
		GetWorldTimerManager().ClearTimer(GameRoundTimerHandle);

		if (CurrentRound + 1 <= GameData.RoundsNum)
		{
			CurrentRound++;
			ResetPlayers();
			StartRound();
		}
		else
		{
			GameOver();
		}
	}
}

void AShooterGameModeBase::ResetPlayers()
{
	if (!GetWorld())
		return;

	for (auto It = GetWorld()->GetControllerIterator(); It; It++)
	{
		ResetOnePlayer(It->Get());
	}
}

void AShooterGameModeBase::ResetOnePlayer(AController* Controller)
{
	if (Controller && Controller->GetPawn())
	{
		Controller->GetPawn()->Reset();
	}
	RestartPlayer(Controller);
	SetPlayerColor(Controller);
}

void AShooterGameModeBase::CreateTeamsInfo()
{
	if (!GetWorld())
		return;

	// logic for 2 teams

	int32 TeamID = 1;
	for (auto It = GetWorld()->GetControllerIterator(); It; It++)
	{
		const auto Controller = It->Get();
		if (!Controller)
			continue;

		const auto PlayerState = Cast<AShooterPlayerState>(Controller->PlayerState);
		if (!PlayerState)
			continue;

		PlayerState->SetTeamID(TeamID);
		PlayerState->SetTeamColor(DetermineColorByTeamID(TeamID));
		PlayerState->SetPlayerName(Controller->IsPlayerController() ? "Player" : "Bot"); // TODO: bots name array
		SetPlayerColor(Controller);

		TeamID = TeamID == 1 ? 2 : 1;
	}
}

FLinearColor AShooterGameModeBase::DetermineColorByTeamID(int32 TeamID) const
{
	if (TeamID - 1 < GameData.TeamColors.Num())
	{
		return GameData.TeamColors[TeamID - 1];
	}
	return GameData.DefaultTeamColor;
}

void AShooterGameModeBase::SetPlayerColor(AController* Controller)
{
	if (!Controller)
		return;

	const auto Character = Cast<AShooterBaseCharacter>(Controller->GetPawn());
	if (!Character)
		return;

	const auto PlayerState = Cast<AShooterPlayerState>(Controller->PlayerState);
	if (!PlayerState)
		return;

	Character->SetPlayerColor(PlayerState->GetTeamColor());
}

void AShooterGameModeBase::LogPlayerInfo()
{
	if (!GetWorld())
		return;

	for (auto It = GetWorld()->GetControllerIterator(); It; It++)
	{
		const auto Controller = It->Get();
		if (!Controller)
			continue;

		const auto PlayerState = Cast<AShooterPlayerState>(Controller->PlayerState);
		if (!PlayerState)
			continue;

		PlayerState->LogInfo();
	}
}

void AShooterGameModeBase::StartRespawn(AController* Controller)
{
	const auto RespawnAvailable = RoundCountDown > GameData.RespawnTime * 2;
	if (!RespawnAvailable)
		return;

	const auto RespawnComponent = ShooterUtils::GetShooterPlayerComponent<UShooterRespawnComponent>(Controller);
	if (!RespawnComponent)
		return;

	RespawnComponent->Respawn(GameData.RespawnTime);
}

void AShooterGameModeBase::GameOver()
{
	LogPlayerInfo();

	for (auto Pawn : TActorRange<APawn>(GetWorld()))
	{
		if (Pawn)
		{
			Pawn->TurnOff();
			Pawn->DisableInput(nullptr);
		}
	}
	SetMatchState(EShooterMatchState::GameOver);
}

void AShooterGameModeBase::SetMatchState(EShooterMatchState State)
{
	if (MatchState == State)
		return;

	MatchState = State;
	OnMatchStateChanged.Broadcast(MatchState);
}

void AShooterGameModeBase::StopAllFire()
{
	for (auto Pawn : TActorRange<APawn>(GetWorld()))
	{
		const auto WeaponComponent = ShooterUtils::GetShooterPlayerComponent<UShooterWeaponComponent>(Pawn);
		if (!WeaponComponent)
			continue;

		WeaponComponent->StopFire();
		WeaponComponent->Zoom(false);
	}
}
