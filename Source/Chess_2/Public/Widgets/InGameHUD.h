// Copyright (©) 2024, Van Phan. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InGameHUD.generated.h"

UENUM(BlueprintType) 
enum EEndGameState : int8
{
  Victory     		UMETA(DisplayName = "Victory"),
  Defeat     		UMETA(DisplayName = "Defeat"),
  Draw   			UMETA(DisplayName = "Draw"),
  EndGameStateNone 	UMETA(DisplayName = "EndGameStateNone"),
};
/**
 * 
 */
UCLASS()
class CHESS_2_API UInGameHUD : public UUserWidget
{
	GENERATED_BODY()

public: 
	void StartGame(bool isWhite);
	void SwapTurns();
	void GameOver(EEndGameState PlayerEndState);
	void ShowPromotionPopup(class ABaseChessPiece* PromotedPawn);
protected: 
	virtual bool Initialize() override;
private:
	UPROPERTY(meta = (BindWidgetAnim), Transient, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
    UWidgetAnimation* StartGameWidgetAnim;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess="true"))
	class UTextBlock* PlayerTurnState; // Displays if the user is in check/their turn/draw, checkmated

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess="true"))
	class UOverlay* PawnPromotionOverlay; 

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess="true"))
	class UButton* QueenPromotion;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess="true"))
	class UButton* KnightPromotion;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess="true"))
	class UButton* RookPromotion;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess="true"))
	class UButton* BishopPromotion;

	UFUNCTION()
	void OnQueenButtonClicked();
	UFUNCTION()
	void OnKnightButtonClicked();
	UFUNCTION()
	void OnRookButtonClicked();
	UFUNCTION()
	void OnBishopButtonClicked();
};
