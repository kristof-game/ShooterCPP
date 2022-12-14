// ShooterGame. All Rights Reserved.

#include "Player/ShooterBaseCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/ShooterCharacterMovementComp.h"
#include "Components/ShooterHealthComponent.h"
#include "GameFramework/Controller.h"
#include "Components/ShooterWeaponComponent.h"
#include "Components/CapsuleComponent.h"
#include "Sound/SoundCue.h"
#include "ShooterGameModeBase.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogBaseCharacter, All, All);

AShooterBaseCharacter::AShooterBaseCharacter(const FObjectInitializer& ObjInit)
	: Super(ObjInit.SetDefaultSubobjectClass<UShooterCharacterMovementComp>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = true;

	HealthComponent = CreateDefaultSubobject<UShooterHealthComponent>("HealthComponent");
	WeaponComponent = CreateDefaultSubobject<UShooterWeaponComponent>("WeaponComponent");
}

void AShooterBaseCharacter::BeginPlay()
{
	Super::BeginPlay();

	check(HealthComponent);
	check(GetCharacterMovement());
	check(GetMesh());

	OnHealthChanged(HealthComponent->GetHealth(), 0.0f);
	HealthComponent->OnDeath.AddUObject(this, &AShooterBaseCharacter::OnDeath);
	HealthComponent->OnHealthChanged.AddUObject(this, &AShooterBaseCharacter ::OnHealthChanged);

	LandedDelegate.AddDynamic(this, &AShooterBaseCharacter::OnGroundLanded);
}

void AShooterBaseCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const auto Health = HealthComponent->GetHealth();
}

void AShooterBaseCharacter::TurnOff()
{
	WeaponComponent->StopFire();
	WeaponComponent->Zoom(false);
	Super::TurnOff();
}

void AShooterBaseCharacter::Reset()
{
	WeaponComponent->StopFire();
	WeaponComponent->Zoom(false);
	Super::Reset();
}

bool AShooterBaseCharacter::IsRunning() const
{
	return false;
}

float AShooterBaseCharacter::GetMovementDirection() const
{
	if (GetVelocity().IsZero())
		return 0.0f;
	const auto VelocityNormal = GetVelocity().GetSafeNormal();
	const auto AngleBetween = FMath::Acos(FVector::DotProduct(GetActorForwardVector(), VelocityNormal));
	const auto CrossProduct = FVector::CrossProduct(GetActorForwardVector(), VelocityNormal);
	const auto Degrees = FMath::RadiansToDegrees(AngleBetween);
	return CrossProduct.IsZero() ? Degrees : Degrees * FMath::Sign(CrossProduct.Z);
}

void AShooterBaseCharacter::SetPlayerColor(const FLinearColor& Color)
{
	const auto MaterialInstance = GetMesh()->CreateAndSetMaterialInstanceDynamic(0);
	if (!MaterialInstance)
		return;

	MaterialInstance->SetVectorParameterValue(MaterialColorName, Color);
}

void AShooterBaseCharacter::OnDeath()
{
	GetCharacterMovement()->DisableMovement();

	SetLifeSpan(LifeSpanOnDeath);
	GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	WeaponComponent->StopFire();

	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetMesh()->SetSimulatePhysics(true);

	UGameplayStatics::PlaySoundAtLocation(GetWorld(), DeathSound, GetActorLocation());
}

void AShooterBaseCharacter::OnHealthChanged(float Health, float HealthDelta)
{
}

void AShooterBaseCharacter::FellOutOfWorld(const UDamageType& dmgType)
{
	//const auto GameMode = Cast<AShooterGameModeBase>(GetWorld()->GetAuthGameMode());
	//OnDeath();
	//GameMode->RespawnRequest(Controller);
	FHitResult HitResult;
	FPointDamageEvent PointDamageEvent;
	PointDamageEvent.HitInfo = HitResult;
	TakeDamage(HealthComponent->GetHealth(), PointDamageEvent, GetController(), this);
}

void AShooterBaseCharacter::OnGroundLanded(const FHitResult& Hit)
{

	const auto FallVelocityZ = -GetVelocity().Z;
	if (FallVelocityZ < LandedDamageVelocity.X)
		return;

	const auto FinalDamage = FMath::GetMappedRangeValueUnclamped(LandedDamageVelocity, LandedDamage, FallVelocityZ);

	FPointDamageEvent PointDamageEvent;
	PointDamageEvent.HitInfo = Hit;
	TakeDamage(FinalDamage, PointDamageEvent, Controller, nullptr);
}
 