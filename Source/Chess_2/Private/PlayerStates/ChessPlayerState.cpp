// Copyright (©) 2024, Van Phan. All rights reserved.


#include "PlayerStates/ChessPlayerState.h"
#include "Net/UnrealNetwork.h"

void AChessPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AChessPlayerState, ChessTeam);
    DOREPLIFETIME(AChessPlayerState, ControlledPawn);
}

