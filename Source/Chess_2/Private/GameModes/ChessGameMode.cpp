// Copyright (©) 2024, Van Phan. All rights reserved.


#include "GameModes/ChessGameMode.h"
#include "PlayerControllers/ChessPlayer.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Math/UnrealMathUtility.h"
#include "Pawns/PlayablePawn.h"
#include "GameStates/ChessGameState.h"
#include "Actors/BaseChessPiece.h"
#include "Net/UnrealNetwork.h"
#include "Actors/BoardSquare.h"

void AChessGameMode::PostLogin(APlayerController *NewPlayer)
{
    bReplicates = true;
    if (AChessPlayer* IncomingPlayer = Cast<AChessPlayer>(NewPlayer))
    {
        AChessGameState* ChessGameState = GetGameState<AChessGameState>();
        if (ChessGameState && !ChessGameState->ChessPlayers.Contains(IncomingPlayer))
        {
            ChessGameState->ChessPlayers.Add(IncomingPlayer);
        }

        if (ChessGameState && ChessGameState->ChessPlayers.Num() == 2)
        {
            FTimerHandle TimerHandle;
            GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &ThisClass::StartGame, 1.0f, false);
        }
    }
}

void AChessGameMode::AddPlayablePawn(APlayablePawn *PlayablePawn)
{
    AChessGameState* ChessGameState = GetGameState<AChessGameState>();
    if (ChessGameState && PlayablePawn && !ChessGameState->ChessPlayablePawns.Contains(PlayablePawn))
    {
        ChessGameState->ChessPlayablePawns.Add(PlayablePawn);
    }
}

void AChessGameMode::AddChessBoard(AChessBoard *ChessBoard)
{
    AChessGameState* ChessGameState = GetGameState<AChessGameState>();
    if (ChessGameState && ChessBoard && ChessGameState->ChessBoard != ChessBoard)
    {
        ChessGameState->ChessBoard = ChessBoard;
    }
}

void AChessGameMode::AddChessPiece(ABaseChessPiece *ChessPiece)
{
    AChessGameState* ChessGameState = GetGameState<AChessGameState>();
    if(ChessPiece && ChessGameState)
    {
        if(ChessPiece->GetChessTeam() == EChessTeam::White)
        {
            if(!ChessGameState->WhitePieces.Contains(ChessPiece)) ChessGameState->WhitePieces.Add(ChessPiece);
        }
        else
        {
            if(!ChessGameState->BlackPieces.Contains(ChessPiece)) ChessGameState->BlackPieces.Add(ChessPiece);
        }
    }
}

void AChessGameMode::AddChessKings(ABaseChessPiece *ChessPiece)
{
    if(!ChessPiece)
    {
        UE_LOG(LogTemp, Display, TEXT("Pieces null?"));
        return;
    }
     
    if(AChessGameState* ChessGameState = GetGameState<AChessGameState>())
    {
        if(ChessPiece->GetChessTeam() == EChessTeam::White)
        {
            ChessGameState->WhiteKing = ChessPiece;
        }
        else
        {
            ChessGameState->BlackKing = ChessPiece;
        }
    }
    else
    {
        UE_LOG(LogTemp, Display, TEXT(" Gamestate broken? "));
    }
}

void AChessGameMode::EndTurn()
{
    if (AChessGameState* ChessGameState = GetGameState<AChessGameState>())
    {
        ChessGameState->WhiteKing->Attackers.Empty();
        ChessGameState->BlackKing->Attackers.Empty();
        ChessGameState->BlackKing->bInCheck = false;
        ChessGameState->WhiteKing->bInCheck = false;

        ChessGameState->CurrentTeamTurn = ChessGameState->CurrentTeamTurn == EChessTeam::White ? EChessTeam::Black : EChessTeam::White;

        TArray<TFuture<void>> Futures;

        if(ChessGameState->CurrentTeamTurn == White)
        {
            // Check Black pieces against White King in parallel
            for (ABaseChessPiece* OpponentPiece : ChessGameState->BlackPieces)
            {
                Futures.Add(Async(EAsyncExecution::ThreadPool, [ChessGameState, OpponentPiece]() {
                    OpponentPiece->IsPiecePinned(ChessGameState->BlackKing->GetCurrentBoardSquare());
                    if (OpponentPiece->CanAttackSquare(ChessGameState->WhiteKing->GetCurrentBoardSquare()))
                    {
                        ChessGameState->WhiteKing->bInCheck = true;
                        ChessGameState->WhiteKing->Attackers.Add(OpponentPiece);
                    }
                }));
            }
        }
        else
        {
            // Check White pieces against Black King in parallel
            for (ABaseChessPiece* OpponentPiece : ChessGameState->WhitePieces)
            {
                Futures.Add(Async(EAsyncExecution::ThreadPool, [ChessGameState, OpponentPiece]() {
                    OpponentPiece->IsPiecePinned(ChessGameState->WhiteKing->GetCurrentBoardSquare());
                    if (OpponentPiece->CanAttackSquare(ChessGameState->BlackKing->GetCurrentBoardSquare()))
                    {
                        ChessGameState->BlackKing->bInCheck = true;
                        ChessGameState->BlackKing->Attackers.Add(OpponentPiece);
                    }
                }));
            }
        }

        // Wait for all tasks to finish
        for (TFuture<void>& Future : Futures)
        {
            Future.Wait();
        }

        for(APlayablePawn* PlayerPawn : ChessGameState->ChessPlayablePawns)
        {
            if(PlayerPawn) PlayerPawn->OnTurnChange(); // Handles disabling stray data on the player pawn
        }

        // Check for in check-mate 
        bool CurrentTeamInDraw = false;
        bool CurrentTeamInCheckmate = false;

        if (ChessGameState->BlackKing->bInCheck) {
            if (ChessGameState->BlackKing->GetNumberOfValidMoves() == 0) {
                // Check if any Black pieces have valid moves
                bool hasValidMove = false;
                for (ABaseChessPiece* PlayerPiece : ChessGameState->BlackPieces) {
                    if (PlayerPiece->GetNumberOfValidMoves() > 0) {
                        hasValidMove = true;
                        break;
                    }
                }

                if (!hasValidMove) {
                    CurrentTeamInCheckmate = true; // Black is checkmated
                }
            }
        } else if (ChessGameState->WhiteKing->bInCheck) {
            if (ChessGameState->WhiteKing->GetNumberOfValidMoves() == 0) {
                // Check if any White pieces have valid moves
                bool hasValidMove = false;
                for (ABaseChessPiece* PlayerPiece : ChessGameState->WhitePieces) {
                    if (PlayerPiece->GetNumberOfValidMoves() > 0) {
                        hasValidMove = true;
                        break;
                    }
                }

                if (!hasValidMove) {
                    CurrentTeamInCheckmate = true; // White is checkmated
                }
            }
        }

        // Check for stalemate
        if (!CurrentTeamInCheckmate) {
            if (!ChessGameState->BlackKing->bInCheck) {
                bool hasValidMove = false;
                for (ABaseChessPiece* PlayerPiece : ChessGameState->BlackPieces) {
                    if (PlayerPiece->GetNumberOfValidMoves() > 0) {
                        hasValidMove = true;
                        break;
                    }
                }
                if (ChessGameState->BlackKing->GetNumberOfValidMoves() == 0 && !hasValidMove) {
                    CurrentTeamInDraw = true; // Black is in stalemate
                }
            }

            if (!ChessGameState->WhiteKing->bInCheck) {
                bool hasValidMove = false;
                for (ABaseChessPiece* PlayerPiece : ChessGameState->WhitePieces) {
                    if (PlayerPiece->GetNumberOfValidMoves() > 0) {
                        hasValidMove = true;
                        break;
                    }
                }
                if (ChessGameState->WhiteKing->GetNumberOfValidMoves() == 0 && !hasValidMove) {
                    CurrentTeamInDraw = true; // White is in stalemate
                }
            }
        }

        FTimerHandle TimerHandle;
        if(CurrentTeamInCheckmate)
        {
            UE_LOG(LogTemp, Display, TEXT("Checkmate fires"));
            ChessGameState->CurrentEndGameState = EEndGameState::Defeat;

        }
        else if(CurrentTeamInDraw)
        {
            ChessGameState->CurrentEndGameState = EEndGameState::Draw;
        }
        else
        {
            ChessGameState->CurrentEndGameState = EEndGameState::EndGameStateNone;
        }

        // Update UI after processing
        GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this]()
        {
            AChessGameState* ChessGameState = GetGameState<AChessGameState>();
            ChessGameState->MulticastUpdateUI(0);
            ChessGameState->MulticastUpdateUI(1);
        }, 0.1f, false);

        
    }
}

void AChessGameMode::PromotePawn(EPromotionType ChosenPiece)
{
    AChessGameState* ChessGameState = GetGameState<AChessGameState>();

    if (!ChessGameState->CurrentPromotingPawn)
    {
        UE_LOG(LogTemp, Warning, TEXT("No pawn available for promotion."));
        return;
    }

    // Get the location and rotation of the current pawn to place the new piece correctly
    FVector PawnLocation = ChessGameState->CurrentPromotingPawn->GetActorLocation();
    FRotator PawnRotation = ChessGameState->CurrentPromotingPawn->GetActorRotation();

    TSubclassOf<ABaseChessPiece> PieceToSpawn = nullptr;

    switch(ChosenPiece)
    {
        case Queen:
            UE_LOG(LogTemp, Display, TEXT(" Queen promotion "));
            PieceToSpawn = QueenPieceClass;
            break;
        case Knight:
            UE_LOG(LogTemp, Display, TEXT(" Knight promotion "));
            PieceToSpawn = KnightPieceClass;
            break;
        case Rook:
            UE_LOG(LogTemp, Display, TEXT(" Rook promotion "));
            PieceToSpawn = RookPieceClass;
            break;
        case Bishop:
            UE_LOG(LogTemp, Display, TEXT(" Bishop promotion "));
            PieceToSpawn = BishopPieceClass;
            break; 
        default:
            UE_LOG(LogTemp, Warning, TEXT("Invalid promotion type"));
            break;
    }

    if (PieceToSpawn)
    {
        // Spawn the new piece at the pawn's location
        ABaseChessPiece* NewPiece = GetWorld()->SpawnActor<ABaseChessPiece>(PieceToSpawn, PawnLocation, PawnRotation);
        if (NewPiece)
        {
            UE_LOG(LogTemp, Display, TEXT("%s successfully spawned."), *NewPiece->GetName());

            // Optionally, set any additional properties for the new piece here
            NewPiece->CurrentBoardSquare = ChessGameState->CurrentPromotingPawn->GetCurrentBoardSquare();
            NewPiece->GetCurrentBoardSquare()->Occupant = NewPiece;
            // Destroy the original pawn
            ChessGameState->CurrentTeamTurn == White ? ChessGameState->WhitePieces.Remove(ChessGameState->CurrentPromotingPawn) : ChessGameState->BlackPieces.Remove(ChessGameState->CurrentPromotingPawn);
            ChessGameState->CurrentPromotingPawn->Destroy();
            NewPiece->InitAfterPromotion(ChessGameState->CurrentTeamTurn);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Failed to spawn the new piece."));
        }
    }

    EndTurn();
}

void AChessGameMode::StartGame()
{
    if(AChessGameState* ChessGameState = GetGameState<AChessGameState>())
    {
        ChessGameState->StartChessGame();
    }
}