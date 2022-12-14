// ShooterGame. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Pickups/ShooterAmmoPickup.h"
#include "ShooterHealthPickup.generated.h"

UCLASS()
class SHOOTERCPP_API AShooterHealthPickup : public AShooterAmmoPickup
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup", meta = (ClampMin = "1.0", ClampMax = "1000.0"))
	float HealthAmmount = 50.0f;

private:
	virtual bool GivePickupTo(APawn* PlayerPawn) override;
};
