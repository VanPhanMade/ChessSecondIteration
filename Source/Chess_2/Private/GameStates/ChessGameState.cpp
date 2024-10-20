// Copyright (©) 2024, Van Phan. All rights reserved.


#include "GameStates/ChessGameState.h"
#include "Pawns/PlayablePawn.h"
#include "PlayerControllers/ChessPlayer.h"
#include "Net/UnrealNetwork.h"
#include "PlayerStates/ChessPlayerState.h"


void AChessGameState::StartChessGame()
{
    if (FMath::RandRange(0, 1))
    {
        WhitePlayer = ChessPlayers[0];
        BlackPlayer = ChessPlayers[1];
    }
    else
    {
        WhitePlayer = ChessPlayers[1];
        BlackPlayer = ChessPlayers[0];
    }

    AChessPlayerState* WhitePlayerState = WhitePlayer->GetPlayerState<AChessPlayerState>();
    AChessPlayerState* BlackPlayerState = BlackPlayer->GetPlayerState<AChessPlayerState>();

    if (!WhitePlayerState || !BlackPlayerState) return;

    WhitePlayerState->ChessTeam = EChessTeam::White;
    BlackPlayerState->ChessTeam = EChessTeam::Black;


    if(ChessPlayablePawns[0]->GetChessTeam() == EChessTeam::White)
    {
        // Possess the correct pawns
        WhitePlayer->Possess(Cast<APawn>(ChessPlayablePawns[0]));
        BlackPlayer->Possess(Cast<APawn>(ChessPlayablePawns[1]));
    }
    else
    {
        WhitePlayer->Possess(Cast<APawn>(ChessPlayablePawns[1]));
        BlackPlayer->Possess(Cast<APawn>(ChessPlayablePawns[0]));
    }

    CurrentTeamTurn = EChessTeam::White;
    CurrentEndGameState = EEndGameState::EndGameStateNone;

    for (short i = 0; i < 2; i++)
    {
        FTimerHandle TimerHandle;
        GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this, i]() { MulticastUpdateUI(i); }, 0.5f, false);
    }

}

void AChessGameState::MulticastUpdateUI_Implementation(int clientIndex)
{
    if (ChessPlayers.IsValidIndex(clientIndex) && ChessPlayers[clientIndex])
    switch(CurrentEndGameState)
    {
        case Victory:
            if(ChessPlayers[clientIndex]->GetPlayerState<AChessPlayerState>()->ChessTeam == CurrentTeamTurn)
            {
                ChessPlayers[clientIndex]->UpdateUIEndGame(EEndGameState::Victory);
            }
            else
            {
                ChessPlayers[clientIndex]->UpdateUIEndGame(EEndGameState::Defeat);
            }
        break;
        case Defeat:
            if(ChessPlayers[clientIndex]->GetPlayerState<AChessPlayerState>()->ChessTeam == CurrentTeamTurn)
            {
                ChessPlayers[clientIndex]->UpdateUIEndGame(EEndGameState::Defeat);
            }
            else
            {
                ChessPlayers[clientIndex]->UpdateUIEndGame(EEndGameState::Victory);
            }
        break;
        case Draw:
            ChessPlayers[clientIndex]->UpdateUIEndGame(EEndGameState::Draw);
        break;
        default:
            ChessPlayers[clientIndex]->UpdateUI();
        break;

    }
    
}

void AChessGameState::PromotePawn(ABaseChessPiece *Pawn)
{
    AChessPlayer* CurrentPromotingPlayer = CurrentTeamTurn == White ? WhitePlayer : BlackPlayer;
    if(Pawn && CurrentPromotingPlayer )
    {
        CurrentPromotingPawn = Pawn;
        CurrentPromotingPlayer->Client_OpenPromotionUI(Pawn);
    }
}

void AChessGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AChessGameState, ChessPlayers);
    DOREPLIFETIME(AChessGameState, ChessPlayablePawns);
    DOREPLIFETIME(AChessGameState, WhitePlayer);
    DOREPLIFETIME(AChessGameState, BlackPlayer);
    DOREPLIFETIME(AChessGameState, CurrentTeamTurn);
    DOREPLIFETIME(AChessGameState, ChessBoard);
    DOREPLIFETIME(AChessGameState, WhitePieces);
    DOREPLIFETIME(AChessGameState, BlackPieces);
    DOREPLIFETIME(AChessGameState, WhiteKing);
    DOREPLIFETIME(AChessGameState, BlackKing);
    DOREPLIFETIME(AChessGameState, LastTwoMovePawn);
    DOREPLIFETIME(AChessGameState, CurrentPromotingPawn);
    DOREPLIFETIME(AChessGameState, CurrentEndGameState);
}

void AChessGameState::BeginPlay()
{
    Super::BeginPlay();
    FTimerHandle TimerHandle;
    GetWorldTimerManager().SetTimer(TimerHandle, this, &AChessGameState::AssignKingToAllPieces, 0.5f, false);
}

void AChessGameState::AssignKingToAllPieces()
{
    if(!WhiteKing || !BlackKing)
    {
        UE_LOG(LogTemp, Display, TEXT(" Kings not added yet. "));
        return;
    }
    for(ABaseChessPiece* Piece : WhitePieces)
    {
        Piece->King = WhiteKing;
    }
    for(ABaseChessPiece* Piece : BlackPieces)
    {
        Piece->King = BlackKing;
    }
}
