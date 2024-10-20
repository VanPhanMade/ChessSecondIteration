// Copyright (©) 2024, Van Phan. All rights reserved.


#include "AnimInstances/ChessHandAnimInstance.h"
#include "Pawns/PlayablePawn.h"

void UChessHandAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
    Super::NativeUpdateAnimation(DeltaTime);

    Player = Player ? Player : Cast<APlayablePawn>(TryGetPawnOwner());
    if(!Player) return;

    bIsHoldingChessPiece = IsValid(Player->GetHeldPiece());
}