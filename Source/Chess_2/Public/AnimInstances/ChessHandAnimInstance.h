// Copyright (©) 2024, Van Phan. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "ChessHandAnimInstance.generated.h"

/**
 * 
 */
UCLASS()
class CHESS_2_API UChessHandAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public: 
 
protected: 
	virtual void NativeUpdateAnimation(float DeltaTime) override;
 
private:
	UPROPERTY(BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	class APlayablePawn* Player;

	UPROPERTY(BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	bool bIsHoldingChessPiece;

	
};
