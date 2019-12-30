#include "GoKartMovementReplicator.h"
#include "UnrealNetwork.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"


UGoKartMovementReplicator::UGoKartMovementReplicator()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicated(true);

}

void UGoKartMovementReplicator::BeginPlay()
{
	Super::BeginPlay();

	MovementComponent = GetOwner()->FindComponentByClass<UGoKartMovementComponent>();
	
}


void UGoKartMovementReplicator::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (MovementComponent == nullptr) return;

	FGoKartMove LastMove = MovementComponent->GetLastMove();

	if (GetOwnerRole() == ROLE_AutonomousProxy)
	{
		UnacknowloegedMoves.Add(LastMove);
		Server_SendMove(LastMove);
	}

	//We are the server and in control of the pawn
	if (GetOwner()->GetRemoteRole() == ROLE_SimulatedProxy)
	{
		UpdateServerState(LastMove);
	}

	if (GetOwnerRole() == ROLE_SimulatedProxy)
	{
		ClientTick(DeltaTime);
	}

}

void UGoKartMovementReplicator::ClearAcknowledgedMoves(FGoKartMove LastMove)
{
	TArray<FGoKartMove> NewMoves;

	for (const FGoKartMove& Move : UnacknowloegedMoves)
	{
		if (Move.Time > LastMove.Time)
		{
			NewMoves.Add(Move);
		}
	}

	UnacknowloegedMoves = NewMoves;
}

void UGoKartMovementReplicator::UpdateServerState(const FGoKartMove& Move)
{
	ServerState.LastMove = Move;
	ServerState.Transform = GetOwner()->GetActorTransform();
	ServerState.Velocity = MovementComponent->GetVelocity();
}

FHermiteCubicSpline UGoKartMovementReplicator::CreateSpline()
{
	FHermiteCubicSpline Spline;

	Spline.TargetLocation = ServerState.Transform.GetLocation();
	Spline.StartLocation = ClientStartTransform.GetLocation();
	Spline.StartDerivative = ClientStartVelocity * VelocityToDerivative();
	Spline.TargetDerivative = ServerState.Velocity * VelocityToDerivative();

	return Spline;
}

float UGoKartMovementReplicator::VelocityToDerivative()
{
	return ClientTimeBetweenLastUpdates * 100;
}

void UGoKartMovementReplicator::ClientTick(float DeltaTime)
{
	ClientTimesinceUpdate += DeltaTime;
	
	if (ClientTimeBetweenLastUpdates < KINDA_SMALL_NUMBER) return;

	if (MovementComponent == nullptr) return;

	float LerpRatio = ClientTimesinceUpdate / ClientTimeBetweenLastUpdates;

	
	FHermiteCubicSpline Spline = CreateSpline();

	InterpolateLocation(Spline, LerpRatio);

	InterpolateVelocity(Spline, LerpRatio);

	InterpolateRotation(LerpRatio);

}

void UGoKartMovementReplicator::InterpolateRotation(float LerpRatio)
{
	FQuat TargetRotation = ServerState.Transform.GetRotation();
	FQuat StartRotation = ClientStartTransform.GetRotation();

	FQuat NewRotation = FQuat::Slerp(StartRotation, TargetRotation, LerpRatio);

	if (MeshOffetRoot != nullptr)
	{
		MeshOffetRoot->SetWorldRotation(NewRotation);
	}
}

void UGoKartMovementReplicator::InterpolateVelocity(const FHermiteCubicSpline& Spline, float LerpRatio)
{
	FVector NewDerivative = Spline.InterpolateDerivative(LerpRatio);
	FVector NewVelocity = NewDerivative / VelocityToDerivative();
	MovementComponent->SetVelocity(NewVelocity);
}

void UGoKartMovementReplicator::InterpolateLocation(const FHermiteCubicSpline& Spline, float LerpRatio)
{
	FVector NewLocation = Spline.InterpolateLocation(LerpRatio);

	if (MeshOffetRoot != nullptr)
	{
		MeshOffetRoot->SetWorldLocation(NewLocation);
	}
}

void UGoKartMovementReplicator::OnRep_ServerState()
{
	switch (GetOwnerRole())
	{
	case ROLE_AutonomousProxy:
		AutonomousProxy_OnRep_ServerState();
		break;
	case ROLE_SimulatedProxy:
		SimulatedProxy_OnRep_ServerState();
		break;
	default:
		break;
	}
}

void UGoKartMovementReplicator::SimulatedProxy_OnRep_ServerState()
{
	if (MovementComponent == nullptr) return;

	ClientTimeBetweenLastUpdates = ClientTimesinceUpdate;
	ClientTimesinceUpdate = 0;

	if (MeshOffetRoot != nullptr)
	{
		ClientStartTransform.SetLocation(MeshOffetRoot->GetComponentLocation());
		ClientStartTransform.SetRotation(MeshOffetRoot->GetComponentQuat());
	}

	ClientStartTransform = GetOwner()->GetActorTransform();
	ClientStartVelocity = MovementComponent->GetVelocity();

	GetOwner()->SetActorTransform(ServerState.Transform);
}

void UGoKartMovementReplicator::AutonomousProxy_OnRep_ServerState()
{
	if (MovementComponent == nullptr) return;

	GetOwner()->SetActorTransform(ServerState.Transform);
	MovementComponent->SetVelocity(ServerState.Velocity);

	ClearAcknowledgedMoves(ServerState.LastMove);

	for (const FGoKartMove& Move : UnacknowloegedMoves)
	{
		MovementComponent->SimulateMove(Move);
	}
}

void UGoKartMovementReplicator::Server_SendMove_Implementation(FGoKartMove Move)
{
	if (MovementComponent == nullptr) return;

	CLientSimulatedTime += Move.DeltaTime;
	MovementComponent->SimulateMove(Move);

	UpdateServerState(Move);
}

bool UGoKartMovementReplicator::Server_SendMove_Validate(FGoKartMove Move)
{
	float ProposedTime = CLientSimulatedTime + Move.DeltaTime;

	bool ClientNotRunningAhead = ProposedTime < GetOwner()->GetWorld()->GetGameState()->GetServerWorldTimeSeconds();
	
	if (!ClientNotRunningAhead)
	{
		UE_LOG(LogTemp, Error, TEXT("Client Running too fast"));
		return false;
	}
	
	if (!Move.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Received invalid move."));
		return false;
	}

	return true;
	
}

void UGoKartMovementReplicator::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UGoKartMovementReplicator, ServerState);
}


