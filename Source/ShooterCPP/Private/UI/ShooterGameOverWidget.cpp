// ShooterGame. All Rights Reserved.

#include "UI/ShooterGameOverWidget.h"
#include "ShooterGameModeBase.h"
#include "Player/ShooterPlayerState.h"
#include "UI/ShooterPlayerStatRowWidget.h"
#include "Components/VerticalBox.h"
#include "ShooterUtils.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"

void UShooterGameOverWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (GetWorld())
	{
		const auto GameMode = Cast<AShooterGameModeBase>(GetWorld()->GetAuthGameMode());
		if (GameMode)
		{
			GameMode->OnMatchStateChanged.AddUObject(this, &UShooterGameOverWidget::OnMatchStateChanged);
		}
	}
	
	if (ResetLevelButton)
	{
		ResetLevelButton->OnClicked.AddDynamic(this, &UShooterGameOverWidget::OnResetLevel);
	}
}

void UShooterGameOverWidget::OnMatchStateChanged(EShooterMatchState State)
{
	if (State == EShooterMatchState::GameOver)
	{
		UpdatePlayersStat();
	}
}

void UShooterGameOverWidget::UpdatePlayersStat()
{
	if (!GetWorld() || !PlayerStatBox)
		return;

	PlayerStatBox->ClearChildren();

	for (auto It = GetWorld()->GetControllerIterator(); It; It++)
	{

		const auto Controller = It->Get();
		if (!Controller)
			continue;

		const auto PlayerState = Cast<AShooterPlayerState>(Controller->PlayerState);
		if (!PlayerState)
			continue;

		const auto PlayerStatRowWidget = CreateWidget<UShooterPlayerStatRowWidget>(GetWorld(), PlayerStatRowWidgetClass);
		if (!PlayerStatRowWidget)
			continue;

		PlayerStatRowWidget->SetPlayerName(FText::FromString(PlayerState->GetPlayerName()));
		PlayerStatRowWidget->SetKills(ShooterUtils::TextFromInt(PlayerState->GetKillsNum()));
		PlayerStatRowWidget->SetDeaths(ShooterUtils::TextFromInt(PlayerState->GetDeathsNum()));
		PlayerStatRowWidget->SetTeam(ShooterUtils::TextFromInt(PlayerState->GetTeamID()));
		PlayerStatRowWidget->SetPlayerIndicatorVisibility(Controller->IsPlayerController());

		PlayerStatBox->AddChild(PlayerStatRowWidget);
	}
}

void UShooterGameOverWidget::OnResetLevel()
{
	const FString CurrentLevelName = UGameplayStatics::GetCurrentLevelName(this);
	UGameplayStatics::OpenLevel(this, FName(CurrentLevelName));
}
