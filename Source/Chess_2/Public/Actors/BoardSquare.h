// Copyright (©) 2024, Van Phan. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BoardSquare.generated.h"

UCLASS()
class CHESS_2_API ABoardSquare : public AActor
{
	GENERATED_BODY()
	
public:	
	ABoardSquare();
	void HighlightSquare(bool isHighlighted); // Material to show a piece can be taken and the current location of a piece
	void ShowValidMoves(bool isShowingMoves); // Shows an indicator for an empty board space a piece can move to

	UPROPERTY(EditAnywhere, Category="Data", Replicated)
	class ABaseChessPiece* Occupant;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const;

private:
	UPROPERTY(EditAnywhere, meta=(AllowPrivateAccess="true"))
	UStaticMeshComponent* MoveToggleMeshComponent;

	UPROPERTY(EditAnywhere, meta=(AllowPrivateAccess="true"))
	UStaticMeshComponent* MeshComponent;

	UPROPERTY(EditAnywhere, Category="Data", meta=(AllowPrivateAccess="true"))
	FString HorizontalCoordinate; 

	UPROPERTY(EditAnywhere, Category="Data", meta=(AllowPrivateAccess="true"))
	int VerticalCoordinate;

	// Materials for move toggle mesh component
	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = "true"))
	class UMaterialInterface* HighlightMaterial;

	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = "true"))
	class UMaterialInterface* NotHighlightedMaterial;

	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = "true"))
	class UMaterialInterface* ValidMoveMaterial;

public:
	FORCEINLINE ABaseChessPiece* GetOccupant() const { return Occupant; } 
	FORCEINLINE FString GetHorizontalCoordinate() const { return HorizontalCoordinate; } 
	FORCEINLINE int GetVerticalCoordinate() const { return VerticalCoordinate; } 

};
