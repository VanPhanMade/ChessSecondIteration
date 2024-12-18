// Copyright (©) 2024, Van Phan. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "Actors/BaseChessPiece.h"
#include "ChessPlayerState.generated.h"

/**
 * 
 */
UCLASS()
class CHESS_2_API AChessPlayerState : public APlayerState
{
	GENERATED_BODY()

public: 
	UPROPERTY(Replicated, BlueprintReadOnly, VisibleAnywhere, Category="Chess")
	TEnumAsByte<EChessTeam> ChessTeam;

	UPROPERTY(Replicated, BlueprintReadOnly, VisibleAnywhere, Category="Chess")
    class APlayablePawn* ControlledPawn;
 
protected: 
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	
};
