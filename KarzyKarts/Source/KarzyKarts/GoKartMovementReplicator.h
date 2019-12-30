// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GoKartMovementComponent.h"
#include "GoKartMovementReplicator.generated.h"

USTRUCT()
struct FGoKartState
{
	GENERATED_BODY()

	UPROPERTY()
	FTransform Transform;
	UPROPERTY()
	FVector Velocity = FVector::ZeroVector;
	UPROPERTY()
	FGoKartMove LastMove;
};

struct FHermiteCubicSpline
{
	FVector StartLocation, StartDerivative, TargetLocation, TargetDerivative;

	FVector InterpolateLocation(float LerpRatio) const
	{
		return FMath::CubicInterp(StartLocation, StartDerivative, TargetLocation, TargetDerivative, LerpRatio);
	}
	FVector InterpolateDerivative(float LerpRatio) const
	{
		return FMath::CubicInterpDerivative(StartLocation, StartDerivative, TargetLocation, TargetDerivative, LerpRatio);
	}
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class KARZYKARTS_API UGoKartMovementReplicator : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UGoKartMovementReplicator();

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	USceneComponent* MeshOffetRoot;
	
	UPROPERTY()
	UGoKartMovementComponent* MovementComponent;

	UPROPERTY(ReplicatedUsing = OnRep_ServerState)
	FGoKartState ServerState;

	TArray<FGoKartMove> UnacknowloegedMoves;

	float ClientTimesinceUpdate;

	float ClientTimeBetweenLastUpdates;

	FTransform ClientStartTransform;

	FVector ClientStartVelocity;

	float CLientSimulatedTime;

	void ClientTick(float DeltaTime);

	void InterpolateLocation(const FHermiteCubicSpline& Spline, float LerpRatio);

	void InterpolateVelocity(const FHermiteCubicSpline& Spline, float LerpRatio);

	void InterpolateRotation(float LerpRatio);

	UFUNCTION(BlueprintCallable)
	void SetMeshOffsetRoot(USceneComponent* Root) { MeshOffetRoot = Root; }

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SendMove(FGoKartMove Move);

	UFUNCTION()
	void OnRep_ServerState();

	UFUNCTION()
	void SimulatedProxy_OnRep_ServerState();

	UFUNCTION()
	void AutonomousProxy_OnRep_ServerState();

	void ClearAcknowledgedMoves(FGoKartMove LastMove);
		
	void UpdateServerState(const FGoKartMove& Move);

	FHermiteCubicSpline CreateSpline();

	float VelocityToDerivative();
};
