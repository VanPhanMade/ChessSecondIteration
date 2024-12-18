// Copyright (©) 2024, Van Phan. All rights reserved.


#include "Actors/ChessBoard.h"
#include "GameStates/ChessGameState.h"

AChessBoard::AChessBoard()
{
	PrimaryActorTick.bCanEverTick = true;
	if (!RootComponent) RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

	ChessBorder = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Chess board"));
	ChessBorder->SetupAttachment(RootComponent);

}

void AChessBoard::ShowValidMove(FString FirstIndex, int SecondIndex)
{
	ChessBoardData[FirstIndex].SecondMap[SecondIndex];
}

void AChessBoard::BeginPlay()
{
	Super::BeginPlay();
	Cast<AChessGameState>(GetWorld()->GetGameState())->ChessBoard = this;
	
}

void AChessBoard::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

