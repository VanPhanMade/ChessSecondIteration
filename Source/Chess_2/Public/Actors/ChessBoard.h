// Copyright (©) 2024, Van Phan. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ChessBoard.generated.h"

USTRUCT(BlueprintType)
struct FMyMapContainer
{
	GENERATED_BODY();

public:
	FMyMapContainer() {}

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<int, class ABoardSquare*> SecondMap;
};

UCLASS()
class CHESS_2_API AChessBoard : public AActor
{
	GENERATED_BODY()
	
public:	
	AChessBoard();

  void ShowValidMove(FString FirstIndex, int SecondIndex);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

private: 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess="true"))
	class UStaticMeshComponent* ChessBorder;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess="true"))
	TMap<FString, FMyMapContainer> ChessBoardData;
public:
	FORCEINLINE TMap<FString, FMyMapContainer>& GetChessBoardData() { return ChessBoardData; } 

};
