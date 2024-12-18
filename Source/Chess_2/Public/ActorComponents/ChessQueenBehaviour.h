// Copyright (©) 2024, Van Phan. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ChessQueenBehaviour.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class CHESS_2_API UChessQueenBehaviour : public UActorComponent
{
	GENERATED_BODY()

public:	
	UChessQueenBehaviour();
protected:
	UFUNCTION()
	virtual void CalculateValidMoves(bool& bInCheck, bool& bIsPinned, bool& bIsProtected, bool& bHasMoved, class ABoardSquare* CurrentBoardSquare);
	UFUNCTION()
	virtual void HideMoves(class ABoardSquare* CurrentBoardSquare);
	UFUNCTION()
	virtual void MovePiece(class ABoardSquare* DesiredMoveSquare);
	UFUNCTION()
	virtual void TakePiece(class ABoardSquare* CurrentBoardSquare);
	UFUNCTION()
	virtual void CanAttackSquare(class ABoardSquare* TargetSquare, bool& bCanAttackSquare);
	UFUNCTION()
	virtual void CalculateIsPinned(class ABoardSquare* KingBoardSquare);
	UFUNCTION()
	virtual void CalculateNumberOfValidMoves(int& numberOfValidMoves);
private:
	UFUNCTION(Server, Reliable)
	void Server_MovePiece(ABoardSquare* DesiredMoveSquare);

		
};
