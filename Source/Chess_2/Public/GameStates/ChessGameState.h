// Copyright (©) 2024, Van Phan. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Actors/BaseChessPiece.h"
#include "Widgets/InGameHUD.h"
#include "ChessGameState.generated.h"

/**
 * 
 */
UCLASS()
class CHESS_2_API AChessGameState : public AGameStateBase
{
	GENERATED_BODY()

public: 
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated)
    TArray<class AChessPlayer*> ChessPlayers;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated)
    TArray<class APlayablePawn*> ChessPlayablePawns;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated)
	class AChessBoard* ChessBoard;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated)
    TArray<class ABaseChessPiece*> WhitePieces;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated)
    TArray<class ABaseChessPiece*> BlackPieces;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated)
	class ABaseChessPiece* WhiteKing;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated)
	class ABaseChessPiece* BlackKing;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated)
	class ABaseChessPiece* LastTwoMovePawn;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated)
	ABaseChessPiece* CurrentPromotingPawn;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated)
	TEnumAsByte<EEndGameState> CurrentEndGameState;

    void StartChessGame();

	// https://dev.epicgames.com/documentation/en-us/unreal-engine/rpcs?application_version=4.27 For referencing RPC calls
    UFUNCTION(NetMulticast, Reliable)
    void MulticastUpdateUI(int clientIndex);

	UPROPERTY(Replicated, BlueprintReadOnly, VisibleAnywhere, Category="Chess")
	TEnumAsByte<EChessTeam> CurrentTeamTurn;

	void PromotePawn(class ABaseChessPiece* Pawn);
 
protected: 
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
private:
	UPROPERTY(VisibleAnywhere, meta=(AllowPrivateAccess="true"), Replicated)
	class AChessPlayer* WhitePlayer;
	UPROPERTY(VisibleAnywhere, meta=(AllowPrivateAccess="true"), Replicated)
	class AChessPlayer* BlackPlayer;

	void AssignKingToAllPieces();
	
public:
	FORCEINLINE AChessBoard* GetChessBoard() const { return ChessBoard; } 
};
