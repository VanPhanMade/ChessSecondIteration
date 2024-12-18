// Copyright (©) 2024, Van Phan. All rights reserved.


#include "Actors/BaseChessPiece.h"
#include "Actors/BoardSquare.h"
#include "Net/UnrealNetwork.h"
#include "GameModes/ChessGameMode.h"

ABaseChessPiece::ABaseChessPiece()
{
	PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;

    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Chess Piece Mesh"));
    SetRootComponent(MeshComponent);
    
    // Only set up physics if this isn't a CDO
    if (!HasAnyFlags(RF_ClassDefaultObject))
    {
        MeshComponent->SetSimulatePhysics(true);
        MeshComponent->SetMassOverrideInKg(NAME_None, 100.0f);
        MeshComponent->SetAngularDamping(10.0f);
        MeshComponent->SetLinearDamping(5.0f);
    }

    MeshComponent->SetIsReplicated(true);
    SetReplicateMovement(false);

    MeshComponent->SetCollisionObjectType(ECC_GameTraceChannel3);
    MeshComponent->SetCollisionResponseToChannel(ECC_GameTraceChannel3, ECR_Ignore);
}

void ABaseChessPiece::HighlightSquare(bool isHighlighted)
{
	CurrentBoardSquare->HighlightSquare(isHighlighted);
}

void ABaseChessPiece::BeginPlay() 
{
	Super::BeginPlay();
	OriginalRotation = GetActorRotation();
	if(AChessGameMode* GameMode = Cast<AChessGameMode>(GetWorld()->GetAuthGameMode())) GameMode->AddChessPiece(this);
}

void ABaseChessPiece::InitAfterPromotion(EChessTeam Team)
{
	if (HasAnyFlags(RF_ClassDefaultObject)) 
    {
        // Skip initialization if this is being called during CDO construction
        return;
    }
	ChessTeam = Team;
	UMaterialInterface* TeamMaterial = ChessTeam == White ? WhiteTeamMaterial : BlackTeamMaterial;
	if (TeamMaterial && MeshComponent)
    {
		
        MeshComponent->SetMaterial(0, TeamMaterial);
    }
	if(AChessGameMode* GameMode = Cast<AChessGameMode>(GetWorld()->GetAuthGameMode())) GameMode->AddChessPiece(this);
	OriginalRotation = GetActorRotation();
}

void ABaseChessPiece::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ABaseChessPiece::PickupChessPiece()
{
	OnPickupPiece.Broadcast();
	ShowValidMoves(true);
}

void ABaseChessPiece::DropChessPiece()
{
	MeshComponent->SetPhysicsLinearVelocity(FVector::ZeroVector);
	MeshComponent->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	SetActorRotation(OriginalRotation);
	SetActorEnableCollision(true);
	SetActorLocation(CurrentBoardSquare->GetActorLocation());
	ShowValidMoves(false);
}

void ABaseChessPiece::MoveChessPiece(ABoardSquare* DesiredMoveSquare)
{
	MeshComponent->SetPhysicsLinearVelocity(FVector::ZeroVector);
	MeshComponent->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	SetActorRotation(OriginalRotation);
	SetActorEnableCollision(true);
	ShowValidMoves(false);
	OnMovePiece.Broadcast(DesiredMoveSquare);
}

void ABaseChessPiece::TakeChessPiece()
{
	OnTakeThisPiece.Broadcast(CurrentBoardSquare);
}

void ABaseChessPiece::ShowValidMoves(bool isShowingValidMoves)
{
	if(isShowingValidMoves)
	{
		OnCalculateValidMoves.Broadcast(bInCheck, bIsPinned, bIsProtected, bHasMoved, CurrentBoardSquare);
	}
	else
	{		
		OnHideMoves.Broadcast(CurrentBoardSquare);
	}
}

int ABaseChessPiece::GetNumberOfValidMoves()
{
	int numberOfValidMoves = 0;
	OnGetNumberOfValidMoves.Broadcast(numberOfValidMoves);
	return numberOfValidMoves;
}

bool ABaseChessPiece::CanAttackSquare(ABoardSquare *TargetSquare)
{	
	bool bCanAttackSquare = false;
	OnCanAttackSquare.Broadcast(TargetSquare, bCanAttackSquare);
    return bCanAttackSquare;
}

bool ABaseChessPiece::IsPiecePinned(ABoardSquare *KingSquareLocation)
{
	OnIsPiecePinned.Broadcast(KingSquareLocation);
    return bIsPinned;
}

void ABaseChessPiece::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABaseChessPiece, bInCheck);
	DOREPLIFETIME(ABaseChessPiece, bIsPinned);
	DOREPLIFETIME(ABaseChessPiece, bIsProtected);
	DOREPLIFETIME(ABaseChessPiece, bHasMoved);
	DOREPLIFETIME(ABaseChessPiece, King);
	DOREPLIFETIME(ABaseChessPiece, CurrentBoardSquare);
	DOREPLIFETIME(ABaseChessPiece, PinDirection);
	DOREPLIFETIME(ABaseChessPiece, Attackers);
}
