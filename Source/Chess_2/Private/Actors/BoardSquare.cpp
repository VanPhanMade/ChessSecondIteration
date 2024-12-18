// Copyright (©) 2024, Van Phan. All rights reserved.


#include "Actors/BoardSquare.h"
#include "Actors/BaseChessPiece.h"
#include "Net/UnrealNetwork.h"

ABoardSquare::ABoardSquare()
{
	PrimaryActorTick.bCanEverTick = true;
	if (!RootComponent) RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Chess Board Square"));
	MeshComponent->SetupAttachment(RootComponent);
	MoveToggleMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Move Toggle Square"));
	MoveToggleMeshComponent->SetupAttachment(RootComponent);
	MoveToggleMeshComponent->SetIsReplicated(false);
	bReplicates = true;
	SetReplicatingMovement(false);
}

void ABoardSquare::HighlightSquare(bool isHighlighted)
{
	if(MoveToggleMeshComponent && HighlightMaterial && NotHighlightedMaterial) 
	MoveToggleMeshComponent->SetMaterial(0, isHighlighted ? HighlightMaterial : NotHighlightedMaterial);
}

void ABoardSquare::ShowValidMoves(bool isShowingMoves)
{
	if(MoveToggleMeshComponent && ValidMoveMaterial && NotHighlightedMaterial && HighlightMaterial)
	{
		MoveToggleMeshComponent->SetMaterial(0, isShowingMoves ? (Occupant ? HighlightMaterial : ValidMoveMaterial) : NotHighlightedMaterial);

		MoveToggleMeshComponent->SetCollisionResponseToChannel(ECC_GameTraceChannel2, isShowingMoves ? ECollisionResponse::ECR_Block : ECollisionResponse::ECR_Ignore);
	}
}

void ABoardSquare::BeginPlay()
{
	Super::BeginPlay();
	
}

void ABoardSquare::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ABoardSquare::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ABoardSquare, Occupant);
}
