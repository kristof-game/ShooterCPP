// ShooterGame. All Rights Reserved.

#include "Player/ShooterPlayerHUDWidget.h"
#include "Components/ShooterHealthComponent.h"
#include "Components/ShooterWeaponComponent.h"
#include "ShooterUtils.h"

float UShooterPlayerHUDWidget::GetHealthPercent() const
{
	const auto HealthComponent = ShooterUtils::GetShooterPlayerComponent<UShooterHealthComponent>(GetOwningPlayerPawn());
	if (!HealthComponent)
		return 0.0f;

	return HealthComponent->GetHealthPercent();
}

bool UShooterPlayerHUDWidget::GetCurrentWeaponUIData(FWeaponUIData& UIData) const
{
	const auto WeaponComponent = ShooterUtils::GetShooterPlayerComponent<UShooterWeaponComponent>(GetOwningPlayerPawn());
	if (!WeaponComponent)
		return false;

	return WeaponComponent->GetCurrentWeaponUIData(UIData);
}

bool UShooterPlayerHUDWidget::GetCurrentWeaponAmmoData(FAmmoData& AmmoData) const
{
	const auto WeaponComponent = ShooterUtils::GetShooterPlayerComponent<UShooterWeaponComponent>(GetOwningPlayerPawn());
	if (!WeaponComponent)
		return false;

	return WeaponComponent->GetCurrentWeaponAmmoData(AmmoData);
}

bool UShooterPlayerHUDWidget::IsPlayerAlive() const
{
	const auto HealthComponent = ShooterUtils::GetShooterPlayerComponent<UShooterHealthComponent>(GetOwningPlayerPawn());
	return HealthComponent && !HealthComponent->IsDead();
}

bool UShooterPlayerHUDWidget::IsPlayerSpectating() const
{
	const auto Controller = GetOwningPlayer();
	return Controller && Controller->GetStateName() == NAME_Spectating;
}

bool UShooterPlayerHUDWidget::Initialize()
{
	if (GetOwningPlayer())
	{
		GetOwningPlayer()->GetOnNewPawnNotifier().AddUObject(this, &UShooterPlayerHUDWidget::OnNewPawn);
		OnNewPawn(GetOwningPlayerPawn());
	}
	return Super::Initialize();
}

void UShooterPlayerHUDWidget::OnHealthChanged(float Health, float HealthDelta)
{
	if (HealthDelta < 0.0f)
	{
		OnTakeDamage();
	}	
}

void UShooterPlayerHUDWidget::OnNewPawn(APawn* NewPawn)
{
	const auto HealthComponent = ShooterUtils::GetShooterPlayerComponent<UShooterHealthComponent>(NewPawn);
	if (HealthComponent)
	{
		HealthComponent->OnHealthChanged.AddUObject(this, &UShooterPlayerHUDWidget::OnHealthChanged);
	}
}
