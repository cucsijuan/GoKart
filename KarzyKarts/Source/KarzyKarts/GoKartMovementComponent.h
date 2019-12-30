// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GoKartMovementComponent.generated.h"

USTRUCT()
struct FGoKartMove
{
	GENERATED_BODY()

	UPROPERTY()
	float Throttle;
	UPROPERTY()
	float SteeringThrow;
	UPROPERTY()
	float DeltaTime;
	UPROPERTY()
	float Time;

	bool IsValid() const 
	{
		return FMath::Abs(Throttle) <= 1 && FMath::Abs(SteeringThrow) <= 1;
	}

};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class KARZYKARTS_API UGoKartMovementComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UGoKartMovementComponent();

	void SetVelocity(FVector InVelocity) { Velocity = InVelocity; }
	FVector GetVelocity() { return Velocity; }

	void SetThrottle(float InThrottle) { Throttle = InThrottle; }

	void SetSteeringThrow(float InSteeringThrow) { SteeringThrow = InSteeringThrow; }
	float GetSteeringThrow() { return SteeringThrow; }

	FGoKartMove GetLastMove() { return LastMove; }

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void SimulateMove(const FGoKartMove& Move);

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

private:
	// The mass of the car (kg).
	UPROPERTY(EditAnywhere)
	float Mass = 1000;

	//Force applied to the car when the throttle is fully down (N).
	UPROPERTY(EditAnywhere)
	float MaxDrivingForce = 10000;

	//Minimum radius of the car turning at full lock (m).
	UPROPERTY(EditAnywhere)
	float MinTurningRadius = 10;

	//Higher means more drag
	UPROPERTY(EditAnywhere)
	float DragCoefficient = 16;

	//Higher means more rolling resistance
	UPROPERTY(EditAnywhere)
	float RollingResistanceCoefficient = 0.015f;

	FVector Velocity = FVector::ZeroVector;

	FGoKartMove LastMove;

	UPROPERTY()
	float Throttle;

	float SteeringThrow;

	FGoKartMove CreateMove(float DeltaTime);

	FVector GetAirResistance();

	FVector GetRollingResistance();

	void UpdateLocationFromVelocity(float DeltaTime);

	void ApplyRotation(float DeltaTime, float SteeringThrow);
};
