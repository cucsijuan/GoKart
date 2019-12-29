#include "GoKartMovementReplicator.h"
#include "UnrealNetwork.h"
#include "GameFramework/Actor.h"


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

void UGoKartMovementReplicator::ClientTick(float DeltaTime)
{
	ClientTimesinceUpdate += DeltaTime;
	
	if (ClientTimeBetweenLastUpdates < KINDA_SMALL_NUMBER) return;

	if (MovementComponent == nullptr) return;

	FVector TargetLocation = ServerState.Transform.GetLocation();
	float LerpRatio = ClientTimesinceUpdate / ClientTimeBetweenLastUpdates;
	FVector StartLocation = ClientStartTransform.GetLocation();
	float VelocityToDerivative = ClientTimeBetweenLastUpdates * 100;
	FVector StartDerivative = ClientStartVelocity * VelocityToDerivative;
	FVector TargetDerivative = ServerState.Velocity * VelocityToDerivative ; 

	FVector NewLocation = FMath::CubicInterp(StartLocation, StartDerivative, TargetLocation, TargetDerivative, LerpRatio);
	GetOwner()->SetActorLocation(NewLocation);

	FVector NewDerivative = FMath::CubicInterpDerivative(StartLocation, StartDerivative, TargetLocation, TargetDerivative, LerpRatio);
	FVector NewVelocity = NewDerivative / VelocityToDerivative;

	MovementComponent->SetVelocity(NewVelocity);

	FQuat TargetRotation = ServerState.Transform.GetRotation();
	FQuat StartRotation = ClientStartTransform.GetRotation();

	FQuat NewRotation = FQuat::Slerp(StartRotation, TargetRotation, LerpRatio);

	GetOwner()->SetActorRotation(NewRotation);
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

	ClientStartTransform = GetOwner()->GetActorTransform();
	ClientStartVelocity = MovementComponent->GetVelocity();
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

	MovementComponent->SimulateMove(Move);

	UpdateServerState(Move);
}

bool UGoKartMovementReplicator::Server_SendMove_Validate(FGoKartMove Move)
{
	return true; //TODO: make better validation
}

void UGoKartMovementReplicator::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UGoKartMovementReplicator, ServerState);
}


