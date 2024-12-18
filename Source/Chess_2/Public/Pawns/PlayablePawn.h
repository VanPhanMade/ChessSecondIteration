// Copyright (©) 2024, Van Phan. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Actors/BaseChessPiece.h"
#include "PlayablePawn.generated.h"

UCLASS()
class CHESS_2_API APlayablePawn : public APawn
{
	GENERATED_BODY()

public:
	APlayablePawn();
	void OnTurnChange();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private: 
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* MovementAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* MouseAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* LeftClickAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* RightClickAction;

	void MoveCallback(const struct FInputActionValue& Value);
	void MouseCallback(const struct FInputActionValue& Value);
	void LeftClickCallback();
	void RightClickCallback();

	UFUNCTION(Server, Reliable)
	void Server_PlacePiece(FHitResult DesiredPlacementHitData, ABaseChessPiece* LastHitPieceFromClient);
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess="true"))
	class USkeletalMeshComponent* HandMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess="true"))
	class USpringArmComponent* SpringArm;

	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* CameraComponent;

	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = "true"))
	class UMaterialInterface* HandMaterial;

	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = "true"))
	TEnumAsByte<enum EChessTeam> PlayerChessTeam;

	UPROPERTY(Replicated, VisibleAnywhere)
    class UPhysicsHandleComponent* PhysicsHandle;

	UPROPERTY(Replicated)
	ABaseChessPiece* LastHitPiece;

	UPROPERTY(Replicated)
	ABaseChessPiece* HeldPiece;

public: 
	FORCEINLINE TEnumAsByte<enum EChessTeam> GetChessTeam() const { return PlayerChessTeam; } 
	FORCEINLINE ABaseChessPiece* GetHeldPiece() const { return HeldPiece; } 
};
