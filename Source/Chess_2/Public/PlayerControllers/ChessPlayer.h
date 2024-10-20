// Copyright (©) 2024, Van Phan. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Actors/BaseChessPiece.h"
#include "Widgets/InGameHUD.h"
#include "ChessPlayer.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnUpdateUI); // Variable type

UENUM(BlueprintType) 
enum EPromotionType : int8
{
  Queen     		UMETA(DisplayName = "Queen"),
  Knight     		UMETA(DisplayName = "Knight"),
  Rook  			UMETA(DisplayName = "Rook"),
  Bishop  			UMETA(DisplayName = "Bishop"),
  PromotionNone  	UMETA(DisplayName = "PromotionNone"),
};
/**
 * 
 */
UCLASS()
class CHESS_2_API AChessPlayer : public APlayerController
{
	GENERATED_BODY()

public: 
	void UpdateUI();

	void UpdateUIEndGame(EEndGameState EndGameState);

	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FOnUpdateUI OnUpdateUI;

	UFUNCTION(Client, Reliable)
	void Client_OpenPromotionUI(ABaseChessPiece* PromotedPawn);

	void PromotionChoice(EPromotionType ChosePromotion);
 
protected: 
	virtual void BeginPlay() override;

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	class UInputMappingContext* DefaultMappingContext;
	
	UPROPERTY(EditAnywhere, Category="UI", meta=(AllowPrivateAccess="true"))
	TSubclassOf<class UInGameHUD> InGameHUDWidget;

	class UInGameHUD* InGameHUDRef;

	bool bStartedGame = false;

	UFUNCTION(Server, Reliable)
	void Server_PromotionChoice(EPromotionType ChosePromotion);
	
};
