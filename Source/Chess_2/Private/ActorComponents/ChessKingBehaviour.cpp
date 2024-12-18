// Copyright (©) 2024, Van Phan. All rights reserved.


#include "ActorComponents/ChessKingBehaviour.h"
#include "Actors/BaseChessPiece.h"
#include "GameStates/ChessGameState.h"
#include "Actors/ChessBoard.h"
#include "Actors/BoardSquare.h"
#include "GameModes/ChessGameMode.h"
#include "ActorComponents/ChessBishopBehaviour.h"
#include "ActorComponents/ChessQueenBehaviour.h"
#include "ActorComponents/ChessRookBehaviour.h"
#include "Utility/ChessHelperFunctions.h"

UChessKingBehaviour::UChessKingBehaviour()
{
	PrimaryComponentTick.bCanEverTick = false;
	ABaseChessPiece* OwningPawn = Cast<ABaseChessPiece>(GetOwner());
	SetIsReplicatedByDefault(true);

	if(OwningPawn)
	{
        if (!OwningPawn->OnCalculateValidMoves.IsBound())
        {
            OwningPawn->OnCalculateValidMoves.AddDynamic(this, &ThisClass::CalculateValidMoves);
        }

        if (!OwningPawn->OnHideMoves.IsBound())
        {
            OwningPawn->OnHideMoves.AddDynamic(this, &ThisClass::HideMoves);
        }

        if (!OwningPawn->OnMovePiece.IsBound())
        {
            OwningPawn->OnMovePiece.AddDynamic(this, &ThisClass::MovePiece);
        }

        if (!OwningPawn->OnTakeThisPiece.IsBound())
        {
            OwningPawn->OnTakeThisPiece.AddDynamic(this, &ThisClass::TakePiece);
        }

        if (!OwningPawn->OnCanAttackSquare.IsBound())
        {
            OwningPawn->OnCanAttackSquare.AddDynamic(this, &ThisClass::CanAttackSquare);
        }

        if (!OwningPawn->OnIsPiecePinned.IsBound())
        {
            OwningPawn->OnIsPiecePinned.AddDynamic(this, &ThisClass::CalculateIsPinned);
        }

        if (!OwningPawn->OnGetNumberOfValidMoves.IsBound())
        {
            OwningPawn->OnGetNumberOfValidMoves.AddDynamic(this, &ThisClass::CalculateNumberOfValidMoves);
        }
	}
}

void UChessKingBehaviour::BeginPlay()
{
	Super::BeginPlay();
	if(AChessGameMode* GameMode = Cast<AChessGameMode>(GetWorld()->GetAuthGameMode())) 
    {
        GameMode->AddChessKings(Cast<ABaseChessPiece>(GetOwner()));
    }
}

void UChessKingBehaviour::CalculateValidMoves(bool &bInCheck, bool &bIsPinned, bool &bIsProtected, bool &bHasMoved, ABoardSquare *CurrentBoardSquare)
{
    if (!CurrentBoardSquare) return;

    if (UWorld* World = GetWorld())
    {
        if (AChessGameState* GameState = Cast<AChessGameState>(World->GetGameState<AGameStateBase>()))
        {
            TMap<FString, FMyMapContainer>& MyBoardData = GameState->GetChessBoard()->GetChessBoardData();
            FString HorizontalCoord = CurrentBoardSquare->GetHorizontalCoordinate();
            int32 VerticalCoord = CurrentBoardSquare->GetVerticalCoordinate();
            TCHAR HorizontalChar = HorizontalCoord[0];
            MyBoardData[HorizontalCoord].SecondMap[VerticalCoord]->HighlightSquare(false);

            ABaseChessPiece* OwnerPiece = Cast<ABaseChessPiece>(GetOwner());

            // Possible moves for the King (one square in all 8 directions)
            TArray<FIntPoint> KingMoves = {
                {1, 0}, {-1, 0}, {0, 1}, {0, -1},    // Horizontal/Vertical
                {1, 1}, {-1, -1}, {1, -1}, {-1, 1}   // Diagonal
            };

            // If the King is in check, prioritize moving out of attack
            if (bInCheck)
            {
                ABaseChessPiece* Attacker = OwnerPiece->Attackers[0];  // First attacker

                for (const FIntPoint& Move : KingMoves)
                {
                    int32 NewVertical = VerticalCoord + Move.Y;
                    TCHAR NewHorizontal = HorizontalChar + Move.X;

                    // Check if the move is within bounds ('A' to 'H' and 1 to 8)
                    if (NewHorizontal >= 'A' && NewHorizontal <= 'H' && NewVertical >= 1 && NewVertical <= 8)
                    {
                        FString NewHorizontalCoord;
                        NewHorizontalCoord.AppendChar(NewHorizontal);

                        if (MyBoardData.Contains(NewHorizontalCoord) && MyBoardData[NewHorizontalCoord].SecondMap.Contains(NewVertical))
                        {
                            ABoardSquare* TargetSquare = MyBoardData[NewHorizontalCoord].SecondMap[NewVertical];

                            // Check if the square is empty or has an opponent's piece
                            if (!TargetSquare->GetOccupant() || TargetSquare->GetOccupant()->GetChessTeam() != OwnerPiece->GetChessTeam())
                            {
                                // Ensure the square is not under attack
                                if (!IsSquareUnderAttack(TargetSquare))
                                {
                                    TargetSquare->ShowValidMoves(true);

                                    // Highlight capture opportunity if it contains an opponent's piece
                                    if (TargetSquare->GetOccupant() && TargetSquare->GetOccupant()->GetChessTeam() != OwnerPiece->GetChessTeam())
                                    {
                                        TargetSquare->HighlightSquare(true);
                                    }
                                }
                            }
                        }
                    }
                }

                return;  // Exit once valid moves out of check are calculated
            }

            // If the King is not in check, calculate normal moves
            for (const FIntPoint& Move : KingMoves)
            {
                int32 NewVertical = VerticalCoord + Move.Y;
                TCHAR NewHorizontal = HorizontalChar + Move.X;

                // Ensure the new position is within board bounds
                if (NewHorizontal >= 'A' && NewHorizontal <= 'H' && NewVertical >= 1 && NewVertical <= 8)
                {
                    FString NewHorizontalCoord;
                    NewHorizontalCoord.AppendChar(NewHorizontal);

                    if (MyBoardData.Contains(NewHorizontalCoord) && MyBoardData[NewHorizontalCoord].SecondMap.Contains(NewVertical))
                    {
                        ABoardSquare* TargetSquare = MyBoardData[NewHorizontalCoord].SecondMap[NewVertical];

                        // Empty square or enemy piece
                        if (!TargetSquare->GetOccupant() ||
                            TargetSquare->GetOccupant()->GetChessTeam() != OwnerPiece->GetChessTeam())
                        {
                            // Verify the square is not under attack
                            if (!IsSquareUnderAttack(TargetSquare))
                            {
                                TargetSquare->ShowValidMoves(true);

                                // Highlight squares with enemy pieces for capturing
                                if (TargetSquare->GetOccupant() && TargetSquare->GetOccupant()->GetChessTeam() != OwnerPiece->GetChessTeam())
                                {
                                    TargetSquare->HighlightSquare(true);
                                }
                            }
                        }
                    }
                }
            }

            if(!bHasMoved)
            {
                // Target squares are the squares the rooks can be on
                if(ABoardSquare* TargetSquare = MyBoardData["A"].SecondMap[VerticalCoord])
                {
                    bool bIsOccupied = MyBoardData["C"].SecondMap[VerticalCoord]->GetOccupant() != nullptr;
                    bool bIsUnderAttack = IsSquareUnderAttack(MyBoardData["C"].SecondMap[VerticalCoord]);

                    if(!bIsOccupied && !bIsUnderAttack)
                    {
                        if(TargetSquare->GetOccupant()->FindComponentByClass<UChessRookBehaviour>() && TargetSquare->GetOccupant()->GetChessTeam() == OwnerPiece->GetChessTeam()
                        && !TargetSquare->GetOccupant()->bHasMoved && ChessHelperFunctions::IsHorizontalLineBetween(TargetSquare, OwnerPiece->GetCurrentBoardSquare(), World))
                        {
                            MyBoardData["C"].SecondMap[VerticalCoord]->ShowValidMoves(true);
                        }
                    }
                }
                
                if(ABoardSquare* TargetSquare = MyBoardData["H"].SecondMap[VerticalCoord])
                {
                    bool bIsOccupied = MyBoardData["G"].SecondMap[VerticalCoord]->GetOccupant() != nullptr;
                    bool bIsUnderAttack = IsSquareUnderAttack(MyBoardData["G"].SecondMap[VerticalCoord]);

                    //UE_LOG(LogTemp, Display, TEXT("Occupant exists: %s, Is under attack: %s"), bIsOccupied ? TEXT("True") : TEXT("False"), bIsUnderAttack ? TEXT("True") : TEXT("False"));
                    if (!bIsOccupied && !bIsUnderAttack)
                    {
                        if(TargetSquare->GetOccupant()->FindComponentByClass<UChessRookBehaviour>() && TargetSquare->GetOccupant()->GetChessTeam() == OwnerPiece->GetChessTeam()
                        && !TargetSquare->GetOccupant()->bHasMoved && ChessHelperFunctions::IsHorizontalLineBetween(TargetSquare, OwnerPiece->GetCurrentBoardSquare(), World))
                        {
                            MyBoardData["G"].SecondMap[VerticalCoord]->ShowValidMoves(true);
                        }
                    }
                }   
            }
        }
    }
}

void UChessKingBehaviour::HideMoves(ABoardSquare *CurrentBoardSquare)
{
	if (!CurrentBoardSquare) return;

    if(const UWorld* World = GetWorld())
    {
        if(const AChessGameState* GameState = Cast<AChessGameState>(World->GetGameState<AGameStateBase>()))
        {
            TMap<FString, FMyMapContainer>& MyBoardData = GameState->GetChessBoard()->GetChessBoardData();
            FString HorizontalCoord = CurrentBoardSquare->GetHorizontalCoordinate();
            int32 VerticalCoord = CurrentBoardSquare->GetVerticalCoordinate();
			MyBoardData[HorizontalCoord].SecondMap[VerticalCoord]->HighlightSquare(false); 
            TCHAR HorizontalChar = HorizontalCoord[0];

            // Define the possible directions for the King (one square in all 8 directions)
            TArray<FIntPoint> KingMoves = {
                {1, 0}, {-1, 0}, {0, 1}, {0, -1},   // Horizontal/Vertical
                {1, 1}, {-1, -1}, {1, -1}, {-1, 1}  // Diagonal
            };

            for (const FIntPoint& Move : KingMoves)
            {
                int32 NewVertical = VerticalCoord + Move.Y;
                TCHAR NewHorizontal = HorizontalChar + Move.X;

                // Check if the new position is within bounds ('A' to 'H' and 1 to 8)
                if (NewHorizontal >= 'A' && NewHorizontal <= 'H' && NewVertical >= 1 && NewVertical <= 8)
                {
                    // Convert NewHorizontal back to FString
                    FString NewHorizontalCoord;
                    NewHorizontalCoord.AppendChar(NewHorizontal);

                    if (MyBoardData.Contains(NewHorizontalCoord) && MyBoardData[NewHorizontalCoord].SecondMap.Contains(NewVertical))
                    {
                        ABoardSquare* TargetSquare = MyBoardData[NewHorizontalCoord].SecondMap[NewVertical];
						TargetSquare->ShowValidMoves(false); 
                    }
                }
            }

            MyBoardData["G"].SecondMap[VerticalCoord]->ShowValidMoves(false);
            MyBoardData["C"].SecondMap[VerticalCoord]->ShowValidMoves(false);
        }
    }
}

void UChessKingBehaviour::MovePiece(ABoardSquare *DesiredMoveSquare)
{
	if(!DesiredMoveSquare) return;
	HideMoves(Cast<ABaseChessPiece>(GetOwner())->GetCurrentBoardSquare());

	if(GetOwner() && GetOwner()->HasAuthority())
	{
		ABaseChessPiece* Parent = Cast<ABaseChessPiece>(GetOwner());
		if(Parent)
		{
			FVector NewReleasePoint = DesiredMoveSquare->GetActorLocation();
			NewReleasePoint.Z = Parent->GetActorLocation().Z;
			Parent->SetActorLocation(NewReleasePoint);
		}

		if(DesiredMoveSquare->GetOccupant()) DesiredMoveSquare->GetOccupant()->TakeChessPiece();

		Parent->GetCurrentBoardSquare()->Occupant = nullptr;
		DesiredMoveSquare->Occupant = Parent;
		Parent->CurrentBoardSquare = DesiredMoveSquare;
        if(!Parent->bHasMoved)
        {
            UWorld* World = GetWorld();
            if(const AChessGameState* GameState = Cast<AChessGameState>(World->GetGameState<AGameStateBase>()))
            {
                TMap<FString, FMyMapContainer>& MyBoardData = GameState->GetChessBoard()->GetChessBoardData();
                // These two squares are only ever revealed when bHasMoved is false if they're castling moves
                if(DesiredMoveSquare == MyBoardData["G"].SecondMap[Parent->GetChessTeam() == White ? 1 : 8])
                {
                    MyBoardData["H"].SecondMap[Parent->GetChessTeam() == White ? 1 : 8]->GetOccupant()->MoveChessPiece(MyBoardData["F"].SecondMap[Parent->GetChessTeam() == White ? 1 : 8]);
                }
                else if(DesiredMoveSquare == MyBoardData["C"].SecondMap[Parent->GetChessTeam() == White ? 1 : 8])
                {
                    MyBoardData["C"].SecondMap[Parent->GetChessTeam() == White ? 1 : 8]->GetOccupant()->MoveChessPiece(MyBoardData["D"].SecondMap[Parent->GetChessTeam() == White ? 1 : 8]);
                }
            }
            if(AChessGameMode* GameMode = Cast<AChessGameMode>(GetWorld()->GetAuthGameMode()))
            {
                GameMode->EndTurn();
            }
        }
        Parent->bHasMoved = true;
        

		if(AChessGameMode* GameMode = Cast<AChessGameMode>(GetWorld()->GetAuthGameMode()))
		{
			GameMode->EndTurn();
		}
	}
	else{
		Server_MovePiece(DesiredMoveSquare);
	}
}

void UChessKingBehaviour::TakePiece(ABoardSquare *DesiredMoveSquare)
{
	if(!GetOwner() || !GetOwner()->HasAuthority()) return;
    if(ABaseChessPiece* Parent = Cast<ABaseChessPiece>(GetOwner()))
    {
        if(AChessGameState* GameState = GetWorld()->GetGameState<AChessGameState>())
        {
            if(Parent->GetChessTeam() == EChessTeam::White)
            {
                GameState->WhitePieces.Remove(Parent);
            }
            else
            {
                GameState->BlackPieces.Remove(Parent);
            }
            // UE_LOG(LogTemp, Display, TEXT("%s has been taken!"), *Parent->GetName());
            Parent->Destroy();
        }
    }
}

void UChessKingBehaviour::CanAttackSquare(ABoardSquare* TargetSquare, bool& bCanAttackSquare)
{
    ABaseChessPiece* Parent = Cast<ABaseChessPiece>(GetOwner());
    if (!Parent || !TargetSquare) return;

    ABoardSquare* CurrentSquare = Parent->GetCurrentBoardSquare();
    if (!CurrentSquare) return;

    if(CurrentSquare == TargetSquare) 
    {
        bCanAttackSquare = false;
        return;
    }

    FString CurrentFileStr = CurrentSquare->GetHorizontalCoordinate();
    int32 CurrentRank = CurrentSquare->GetVerticalCoordinate();

    FString TargetFileStr = TargetSquare->GetHorizontalCoordinate();
    int32 TargetRank = TargetSquare->GetVerticalCoordinate();

    int32 FileDifference = FMath::Abs(TargetFileStr[0] - CurrentFileStr[0]);
    int32 RankDifference = FMath::Abs(TargetRank - CurrentRank);

    // King moves one square in any direction
    if (FileDifference <= 1 && RankDifference <= 1)
    {
        bCanAttackSquare = true;
        return;
    }

    bCanAttackSquare = false;
}

void UChessKingBehaviour::CalculateIsPinned(ABoardSquare *KingBoardSquare)
{
    // no need to check pinned for king
}

void UChessKingBehaviour::CalculateNumberOfValidMoves(int &numberOfValidMoves)
{
    numberOfValidMoves = 0;  // Start with zero valid moves

    if (UWorld* World = GetWorld())
    {
        if (AChessGameState* GameState = Cast<AChessGameState>(World->GetGameState<AGameStateBase>()))
        {   
            ABoardSquare* CurrentBoardSquare = Cast<ABaseChessPiece>(GetOwner())->GetCurrentBoardSquare();
            TMap<FString, FMyMapContainer>& MyBoardData = GameState->GetChessBoard()->GetChessBoardData();
            FString HorizontalCoord = CurrentBoardSquare->GetHorizontalCoordinate();
            int32 VerticalCoord = CurrentBoardSquare->GetVerticalCoordinate();
            TCHAR HorizontalChar = HorizontalCoord[0];

            ABaseChessPiece* OwnerPiece = Cast<ABaseChessPiece>(GetOwner());

            // Possible moves for the King (one square in all 8 directions)
            TArray<FIntPoint> KingMoves = {
                {1, 0}, {-1, 0}, {0, 1}, {0, -1},    // Horizontal/Vertical
                {1, 1}, {-1, -1}, {1, -1}, {-1, 1}   // Diagonal
            };

            // Iterate over possible moves
            for (const FIntPoint& Move : KingMoves)
            {
                int32 NewVertical = VerticalCoord + Move.Y;
                TCHAR NewHorizontal = HorizontalChar + Move.X;

                // Ensure the new position is within board bounds
                if (NewHorizontal >= 'A' && NewHorizontal <= 'H' && NewVertical >= 1 && NewVertical <= 8)
                {
                    FString NewHorizontalCoord;
                    NewHorizontalCoord.AppendChar(NewHorizontal);

                    if (MyBoardData.Contains(NewHorizontalCoord) && MyBoardData[NewHorizontalCoord].SecondMap.Contains(NewVertical))
                    {
                        ABoardSquare* TargetSquare = MyBoardData[NewHorizontalCoord].SecondMap[NewVertical];

                        // Empty square or enemy piece
                        if (!TargetSquare->GetOccupant() ||
                            TargetSquare->GetOccupant()->GetChessTeam() != OwnerPiece->GetChessTeam())
                        {
                            // Ensure the square is not under attack
                            if (!IsSquareUnderAttack(TargetSquare))
                            {
                                // Increment the count of valid moves
                                numberOfValidMoves++;

                                // If we've found at least one valid move, stop further calculations
                                if (numberOfValidMoves >= 1)
                                {
                                    return;
                                }
                            }
                        }
                    }
                }
            }

            // Optionally, handle castling if the king hasn't moved
            if(!OwnerPiece->bHasMoved)
            {
                // Castling check logic (similar to above)
                // If a valid castling move is found, set numberOfValidMoves and return

                // Kingside castling check
                if (ABoardSquare* TargetSquare = MyBoardData["H"].SecondMap[VerticalCoord])
                {
                    bool bIsOccupied = MyBoardData["G"].SecondMap[VerticalCoord]->GetOccupant() != nullptr;
                    bool bIsUnderAttack = IsSquareUnderAttack(MyBoardData["G"].SecondMap[VerticalCoord]);

                    if (!bIsOccupied && !bIsUnderAttack)
                    {
                        if (TargetSquare->GetOccupant()->FindComponentByClass<UChessRookBehaviour>() && TargetSquare->GetOccupant()->GetChessTeam() == OwnerPiece->GetChessTeam()
                            && !TargetSquare->GetOccupant()->bHasMoved && ChessHelperFunctions::IsHorizontalLineBetween(TargetSquare, OwnerPiece->GetCurrentBoardSquare(), World))
                        {
                            // Increment valid moves for castling
                            numberOfValidMoves++;
                            if (numberOfValidMoves >= 1) return;
                        }
                    }
                }

                // Queenside castling check
                if (ABoardSquare* TargetSquare = MyBoardData["A"].SecondMap[VerticalCoord])
                {
                    bool bIsOccupied = MyBoardData["C"].SecondMap[VerticalCoord]->GetOccupant() != nullptr;
                    bool bIsUnderAttack = IsSquareUnderAttack(MyBoardData["C"].SecondMap[VerticalCoord]);

                    if (!bIsOccupied && !bIsUnderAttack)
                    {
                        if (TargetSquare->GetOccupant()->FindComponentByClass<UChessRookBehaviour>() && TargetSquare->GetOccupant()->GetChessTeam() == OwnerPiece->GetChessTeam()
                            && !TargetSquare->GetOccupant()->bHasMoved && ChessHelperFunctions::IsHorizontalLineBetween(TargetSquare, OwnerPiece->GetCurrentBoardSquare(), World))
                        {
                            // Increment valid moves for castling
                            numberOfValidMoves++;
                            if (numberOfValidMoves >= 1) return;
                        }
                    }
                }
            }
        }
    }
}

void UChessKingBehaviour::Server_MovePiece_Implementation(ABoardSquare *DesiredMoveSquare)
{
	if(!DesiredMoveSquare) return;
	ABaseChessPiece* Parent = Cast<ABaseChessPiece>(GetOwner());
	if(Parent)
	{
		FVector NewReleasePoint = DesiredMoveSquare->GetActorLocation();
		NewReleasePoint.Z = Parent->GetActorLocation().Z;
		Parent->SetActorLocation(NewReleasePoint);
	}
    if (const UWorld* World = GetWorld())
	if (AChessGameState* GameState = Cast<AChessGameState>(World->GetGameState<AGameStateBase>()))
	{
		GameState->LastTwoMovePawn = nullptr;
	}

    if(DesiredMoveSquare->GetOccupant()) DesiredMoveSquare->GetOccupant()->TakeChessPiece();

    Parent->GetCurrentBoardSquare()->Occupant = nullptr;
    DesiredMoveSquare->Occupant = Parent;
    Parent->CurrentBoardSquare = DesiredMoveSquare;
    if(!Parent->bHasMoved)
    {
        UWorld* World = GetWorld();
        if(const AChessGameState* GameState = Cast<AChessGameState>(World->GetGameState<AGameStateBase>()))
        {
            TMap<FString, FMyMapContainer>& MyBoardData = GameState->GetChessBoard()->GetChessBoardData();
            // These two squares are only ever revealed when bHasMoved is false if they're castling moves
            if(DesiredMoveSquare == MyBoardData["G"].SecondMap[Parent->GetChessTeam() == White ? 1 : 8])
            {
                MyBoardData["H"].SecondMap[Parent->GetChessTeam() == White ? 1 : 8]->GetOccupant()->MoveChessPiece(MyBoardData["F"].SecondMap[Parent->GetChessTeam() == White ? 1 : 8]);
            }
            else if(DesiredMoveSquare == MyBoardData["C"].SecondMap[Parent->GetChessTeam() == White ? 1 : 8])
            {
                MyBoardData["C"].SecondMap[Parent->GetChessTeam() == White ? 1 : 8]->GetOccupant()->MoveChessPiece(MyBoardData["D"].SecondMap[Parent->GetChessTeam() == White ? 1 : 8]);
            }
        }
        if(AChessGameMode* GameMode = Cast<AChessGameMode>(GetWorld()->GetAuthGameMode()))
        {
            GameMode->EndTurn();
        }
    }
    Parent->bHasMoved = true;

    if(AChessGameMode* GameMode = Cast<AChessGameMode>(GetWorld()->GetAuthGameMode()))
    {
        GameMode->EndTurn();
    }
}

bool UChessKingBehaviour::IsSquareUnderAttack(ABoardSquare* TargetSquare)
{
    if (!TargetSquare)
    {
        UE_LOG(LogTemp, Warning, TEXT("IsSquareUnderAttack: TargetSquare is null"));
        return false;
    }

    if (UWorld* World = GetWorld())
    {
        if (AChessGameState* GameState = Cast<AChessGameState>(World->GetGameState<AGameStateBase>()))
        {
            TArray<ABaseChessPiece*> EnemyPieces = Cast<ABaseChessPiece>(GetOwner())->GetChessTeam() == EChessTeam::White ? GameState->BlackPieces : GameState->WhitePieces;
            TArray<TFuture<bool>> Futures;

            // Launch tasks to check if each piece can attack the square
            for (ABaseChessPiece* EnemyPiece : EnemyPieces)
            {
                if (EnemyPiece)
                {
                    Futures.Add(Async(EAsyncExecution::ThreadPool, [EnemyPiece, TargetSquare]() {
                        return EnemyPiece->CanAttackSquare(TargetSquare);
                    }));
                }
            }

            // Wait for all tasks to complete and check results
            for (TFuture<bool>& Future : Futures)
            {
                if (Future.Get())
                {
                    return true;  // Square is under attack
                }
            }
        }
    }

    return false;
}
