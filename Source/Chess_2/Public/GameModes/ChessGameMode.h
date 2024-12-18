// Copyright (©) 2024, Van Phan. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "PlayerControllers/ChessPlayer.h"
#include "ChessGameMode.generated.h"

/**
 * 
 */
UCLASS()
class CHESS_2_API AChessGameMode : public AGameModeBase
{
	GENERATED_BODY()

public: 
	virtual void AddPlayablePawn(class APlayablePawn* PlayablePawn);
	virtual void AddChessBoard(class AChessBoard* ChessBoard);
	virtual void AddChessPiece(class ABaseChessPiece* ChessPiece);
	virtual void AddChessKings(class ABaseChessPiece* ChessPiece);
	void EndTurn();
	virtual void PromotePawn(EPromotionType ChosenPiece);
 
protected: 
	virtual void PostLogin(APlayerController* NewPlayer) override;
 
private:

	UPROPERTY(EditDefaultsOnly, Category = "Chess Pieces")
    TSubclassOf<class ABaseChessPiece> QueenPieceClass;

    UPROPERTY(EditDefaultsOnly, Category = "Chess Pieces")
    TSubclassOf<class ABaseChessPiece> KnightPieceClass;

    UPROPERTY(EditDefaultsOnly, Category = "Chess Pieces")
    TSubclassOf<class ABaseChessPiece> RookPieceClass;

    UPROPERTY(EditDefaultsOnly, Category = "Chess Pieces")
    TSubclassOf<class ABaseChessPiece> BishopPieceClass;
	
	void StartGame();
};
