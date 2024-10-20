// Copyright (©) 2024, Van Phan. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BaseChessPiece.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams( FOnCalculateValidMovesSignature, 
	bool&, bInCheck, 
	bool&, bIsPinned, 
	bool& , bIsProtected, 
	bool& , bHasMoved, 
	class ABoardSquare* , 
	CurrentBoardSquare);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHideMovesSignature, class ABoardSquare* , CurrentBoardSquare);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMovePieceSignature, class ABoardSquare* , DesiredMoveSquare);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTakeThisPieceSignature, class ABoardSquare* , CurrentBoardSquare);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCanAttackSquareSignature, class ABoardSquare* , TargetSquare, bool&, bCanAttackSquare);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnIsPiecePinnedSignature, class ABoardSquare* , KingSquareLocation);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGetNumberOfValidMovesSignature, int&, numberOfValidMoves);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPickupPieceSignature);

UENUM(BlueprintType) 
enum EChessTeam : int8
{
  Black     UMETA(DisplayName = "Black"),
  White     UMETA(DisplayName = "White"),
  TeamNone  UMETA(DisplayName = "TeamNone"),
};

UENUM(BlueprintType) 
enum EPinDirection : int8
{
  Horizontal    	UMETA(DisplayName = "Horizontal"),
  Vertical      	UMETA(DisplayName = "Vertical"),
  Diagonal   		UMETA(DisplayName = "Diagonal"),
  PinDirectionNone  UMETA(DisplayName = "PinDirectionNone"),
};

UCLASS()
class CHESS_2_API ABaseChessPiece : public AActor
{
	GENERATED_BODY()
	
public:	
	ABaseChessPiece();
	void HighlightSquare(bool isHighlighted);
	virtual void PickupChessPiece(); // Grabs the piece and toggles available chess moves
	virtual void DropChessPiece(); // Returns the piece to where it was previously was on the board
	virtual void MoveChessPiece(class ABoardSquare* DesiredMoveSquare); // Attempts to move piece to desired square of valid
	virtual void TakeChessPiece(); // Called from other chess pieces after a move
	virtual bool CanAttackSquare(class ABoardSquare* TargetSquare);
	virtual bool IsPiecePinned(class ABoardSquare* KingSquareLocation);
	virtual int GetNumberOfValidMoves();

	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FOnCalculateValidMovesSignature OnCalculateValidMoves;

	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FOnHideMovesSignature OnHideMoves;

	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FOnMovePieceSignature OnMovePiece;

	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FOnTakeThisPieceSignature OnTakeThisPiece;

	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FOnCanAttackSquareSignature OnCanAttackSquare;

	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FOnIsPiecePinnedSignature OnIsPiecePinned;

	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FOnGetNumberOfValidMovesSignature OnGetNumberOfValidMoves;

	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FOnPickupPieceSignature OnPickupPiece;
	
	// values updated when board changes and a piece moves
	UPROPERTY(Replicated, VisibleAnywhere)
	bool bInCheck = false;

	UPROPERTY(Replicated, VisibleAnywhere)
	bool bIsPinned = false;

	UPROPERTY(Replicated, VisibleAnywhere)
	bool bIsProtected = false;

	UPROPERTY(Replicated, VisibleAnywhere)
	bool bHasMoved = false; 

	UPROPERTY(Replicated, EditAnywhere)
	class ABaseChessPiece* King;
	
	UPROPERTY(Replicated, EditAnywhere)
	class ABoardSquare* CurrentBoardSquare;

	UPROPERTY(Replicated, EditAnywhere)
	TEnumAsByte<EPinDirection> PinDirection = PinDirectionNone;

	UPROPERTY(Replicated, EditAnywhere)
	TArray<class ABaseChessPiece*> Attackers;

	void InitAfterPromotion(EChessTeam Team);
	
protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void ShowValidMoves(bool isShowingValidMoves);
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess="true"))
	UStaticMeshComponent* MeshComponent;

	UPROPERTY(EditAnywhere, meta=(AllowPrivateAccess="true"))
	TEnumAsByte<EChessTeam> ChessTeam = TeamNone;
	
	FRotator OriginalRotation;

	// Materials for the two teams
    UPROPERTY(EditDefaultsOnly, Category = "Materials", meta=(AllowPrivateAccess="true"))
    UMaterialInterface* WhiteTeamMaterial;

    UPROPERTY(EditDefaultsOnly, Category = "Materials", meta=(AllowPrivateAccess="true"))
    UMaterialInterface* BlackTeamMaterial;

public:
	FORCEINLINE TEnumAsByte<enum EChessTeam> GetChessTeam() const { return ChessTeam; } 
	FORCEINLINE ABoardSquare* GetCurrentBoardSquare() const { return CurrentBoardSquare; } 
};







