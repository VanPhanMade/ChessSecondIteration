// Copyright (©) 2024, Van Phan. All rights reserved.


#include "ActorComponents/ChessQueenBehaviour.h"
#include "Actors/BaseChessPiece.h"
#include "GameStates/ChessGameState.h"
#include "Actors/ChessBoard.h"
#include "Actors/BoardSquare.h"
#include "GameModes/ChessGameMode.h"
#include "ActorComponents/ChessBishopBehaviour.h"
#include "ActorComponents/ChessQueenBehaviour.h"
#include "ActorComponents/ChessRookBehaviour.h"
#include "Utility/ChessHelperFunctions.h"

UChessQueenBehaviour::UChessQueenBehaviour()
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

void UChessQueenBehaviour::CalculateValidMoves(bool &bInCheck, bool &bIsPinned, bool &bIsProtected, bool &bHasMoved, ABoardSquare *CurrentBoardSquare)
{
    if (!CurrentBoardSquare) return;

    if (UWorld* World = GetWorld())
    {
        if (const AChessGameState* GameState = Cast<AChessGameState>(World->GetGameState<AGameStateBase>()))
        {
            TMap<FString, FMyMapContainer>& MyBoardData = GameState->GetChessBoard()->GetChessBoardData();
            FString HorizontalCoord = CurrentBoardSquare->GetHorizontalCoordinate();
            int32 VerticalCoord = CurrentBoardSquare->GetVerticalCoordinate();
            TCHAR HorizontalChar = HorizontalCoord[0];
            MyBoardData[HorizontalCoord].SecondMap[VerticalCoord]->HighlightSquare(false);

            ABaseChessPiece* OwnerPiece = Cast<ABaseChessPiece>(GetOwner());
            ABaseChessPiece* King = (OwnerPiece->GetChessTeam() == EChessTeam::White) ? GameState->WhiteKing : GameState->BlackKing;

            // Queen combines both rook (straight) and bishop (diagonal) moves
            TArray<FIntPoint> QueenDirections = {
                {1, 0},  {-1, 0},   {0, 1},  {0, -1}, // Rook directions (horizontal/vertical)
                {1, 1},  {-1, 1},   {1, -1}, {-1, -1} // Bishop directions (diagonals)
            };

            // If the King is in check
            if (King->bInCheck)
            {
                if (bIsPinned) return; // Can't move if pinned

                // Only allow moves that can block or capture the attacker
                ABaseChessPiece* Attacker = King->Attackers[0];

                for (const FIntPoint& Direction : QueenDirections)
                {
                    int32 Steps = 1;
                    while (true)
                    {
                        int32 NewVertical = VerticalCoord + (Direction.Y * Steps);
                        TCHAR NewHorizontal = HorizontalChar + (Direction.X * Steps);

                        // Check if the new position is within bounds ('A' to 'H' and 1 to 8)
                        if (NewHorizontal < 'A' || NewHorizontal > 'H' || NewVertical < 1 || NewVertical > 8)
                        {
                            break; // Out of bounds
                        }

                        FString NewHorizontalCoord;
                        NewHorizontalCoord.AppendChar(NewHorizontal);

                        if (MyBoardData.Contains(NewHorizontalCoord) && MyBoardData[NewHorizontalCoord].SecondMap.Contains(NewVertical))
                        {
                            ABoardSquare* TargetSquare = MyBoardData[NewHorizontalCoord].SecondMap[NewVertical];

                            // Can capture the attacker
                            if (TargetSquare == Attacker->GetCurrentBoardSquare())
                            {
                                TargetSquare->ShowValidMoves(true);
                                TargetSquare->HighlightSquare(true);
                                break;
                            }
                            // Can block the attack path if it's a straight or diagonal line
                            else if (ChessHelperFunctions::IsHorizontalLineBetween(TargetSquare, Attacker->GetCurrentBoardSquare(), World) 
							&& ChessHelperFunctions::IsHorizontalLineBetween(TargetSquare, King->GetCurrentBoardSquare(), World))
                            {
                                TargetSquare->ShowValidMoves(true);
                            }
							else if(ChessHelperFunctions::IsVerticalLineBetween(TargetSquare, Attacker->GetCurrentBoardSquare(), World)
							&& ChessHelperFunctions::IsVerticalLineBetween(TargetSquare, King->GetCurrentBoardSquare(), World))
							{
								TargetSquare->ShowValidMoves(true);
							}
							else if(ChessHelperFunctions::IsDiagonalLineBetween(TargetSquare, Attacker->GetCurrentBoardSquare(), World) 
							&& ChessHelperFunctions::IsDiagonalLineBetween(TargetSquare, King->GetCurrentBoardSquare(), World))
							{
								TargetSquare->ShowValidMoves(true);
							}	
                            else
                            {
                                break; // No valid block in this direction
                            }
                        }
                        Steps++;
                    }
                }
                return;
            }

            // If the Queen is pinned, restrict movement along the pin line
            if (bIsPinned)
            {
                for (const FIntPoint& Direction : QueenDirections)
                {
                    int32 Steps = 1;

                    while (true)
                    {
                        int32 NewVertical = VerticalCoord + (Direction.Y * Steps);
                        TCHAR NewHorizontal = HorizontalChar + (Direction.X * Steps);

                        // Check if the new position is within bounds
                        if (NewHorizontal < 'A' || NewHorizontal > 'H' || NewVertical < 1 || NewVertical > 8)
                        {
                            break; // Out of bounds
                        }

                        FString NewHorizontalCoord;
                        NewHorizontalCoord.AppendChar(NewHorizontal);

                        if (MyBoardData.Contains(NewHorizontalCoord) && MyBoardData[NewHorizontalCoord].SecondMap.Contains(NewVertical))
                        {
                            ABoardSquare* TargetSquare = MyBoardData[NewHorizontalCoord].SecondMap[NewVertical];

                            // Only allow moves along the pinning line (diagonal or straight)
                            if (ChessHelperFunctions::IsDiagonalLineBetween(OwnerPiece->Attackers[0]->GetCurrentBoardSquare(), TargetSquare, World) ||
                                ChessHelperFunctions::IsHorizontalLineBetween(OwnerPiece->Attackers[0]->GetCurrentBoardSquare(), TargetSquare, World) ||
                                ChessHelperFunctions::IsVerticalLineBetween(OwnerPiece->Attackers[0]->GetCurrentBoardSquare(), TargetSquare, World))
                            {
                                if (!TargetSquare->GetOccupant())
                                {
                                    TargetSquare->ShowValidMoves(true); // Empty square
                                }
                                else if (TargetSquare == OwnerPiece->Attackers[0]->GetCurrentBoardSquare())
                                {
                                    TargetSquare->ShowValidMoves(true); // Capture pinning piece
                                    TargetSquare->HighlightSquare(true);
                                    break;
                                }
                                else
                                {
                                    break; // Blocked by own piece
                                }
                            }
                            else
                            {
                                break; // Invalid move outside the pin line
                            }
                        }
                        Steps++;
                    }
                }
                return;
            }

            // Default Queen movement (not in check or pinned)
            for (const FIntPoint& Direction : QueenDirections)
            {
                int32 Steps = 1;

                while (true)
                {
                    int32 NewVertical = VerticalCoord + (Direction.Y * Steps);
                    TCHAR NewHorizontal = HorizontalChar + (Direction.X * Steps);

                    // Check if the new position is within bounds
                    if (NewHorizontal < 'A' || NewHorizontal > 'H' || NewVertical < 1 || NewVertical > 8)
                    {
                        break; // Out of bounds
                    }

                    FString NewHorizontalCoord;
                    NewHorizontalCoord.AppendChar(NewHorizontal);

                    if (MyBoardData.Contains(NewHorizontalCoord) && MyBoardData[NewHorizontalCoord].SecondMap.Contains(NewVertical))
                    {
                        ABoardSquare* TargetSquare = MyBoardData[NewHorizontalCoord].SecondMap[NewVertical];

                        // Empty square is a valid move
                        if (!TargetSquare->GetOccupant())
                        {
                            TargetSquare->ShowValidMoves(true);
                            Steps++; // Continue in the same direction
                        }
                        else
                        {
                            // Occupied by opponent's piece
                            if (TargetSquare->GetOccupant()->GetChessTeam() != OwnerPiece->GetChessTeam())
                            {
                                TargetSquare->ShowValidMoves(true);
                                TargetSquare->HighlightSquare(true);
                            }
                            break; // Stop after finding a piece
                        }
                    }
                    else
                    {
                        break; // No square found, stop this direction
                    }
                }
            }
        }
    }
}

void UChessQueenBehaviour::HideMoves(ABoardSquare *CurrentBoardSquare)
{
	if (!CurrentBoardSquare) return;

	if (const UWorld* World = GetWorld())
	{
		if (const AChessGameState* GameState = Cast<AChessGameState>(World->GetGameState<AGameStateBase>()))
		{
			TMap<FString, FMyMapContainer>& MyBoardData = GameState->GetChessBoard()->GetChessBoardData();
			FString HorizontalCoord = CurrentBoardSquare->GetHorizontalCoordinate();
			int32 VerticalCoord = CurrentBoardSquare->GetVerticalCoordinate();
			MyBoardData[HorizontalCoord].SecondMap[VerticalCoord]->HighlightSquare(false); 
			TCHAR HorizontalChar = HorizontalCoord[0];

			// Queen combines bishop and rook moves
			TArray<FIntPoint> QueenDirections = {
				// Bishop directions (diagonals)
				{1, 1},  {-1, 1},   {1, -1}, {-1, -1},
				// Rook directions (straight lines)
				{1, 0},  {-1, 0},   {0, 1},  {0, -1}
			};

			for (const FIntPoint& Direction : QueenDirections)
			{
				int32 Steps = 1;

				while (true)
				{
					int32 NewVertical = VerticalCoord + (Direction.Y * Steps);
					TCHAR NewHorizontal = HorizontalChar + (Direction.X * Steps);

					// Check if the new position is within bounds ('A' to 'H' and 1 to 8)
					if (NewHorizontal < 'A' || NewHorizontal > 'H' || NewVertical < 1 || NewVertical > 8)
					{
						break; // Out of bounds, stop searching this direction
					}

					// Convert NewHorizontal back to FString
					FString NewHorizontalCoord;
					NewHorizontalCoord.AppendChar(NewHorizontal);

					// Check if this square exists on the board
					if (MyBoardData.Contains(NewHorizontalCoord) && MyBoardData[NewHorizontalCoord].SecondMap.Contains(NewVertical))
					{
						ABoardSquare* TargetSquare = MyBoardData[NewHorizontalCoord].SecondMap[NewVertical];
						TargetSquare->ShowValidMoves(false);
						Steps++;
					}
					else
					{
						break; // No square found, stop searching this direction
					}
				}
			}
		}
	}
}

void UChessQueenBehaviour::MovePiece(ABoardSquare *DesiredMoveSquare)
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

        if (const UWorld* World = GetWorld())
        if (AChessGameState* GameState = Cast<AChessGameState>(World->GetGameState<AGameStateBase>()))
        {
            GameState->LastTwoMovePawn = nullptr;
        }

		if(DesiredMoveSquare->GetOccupant()) DesiredMoveSquare->GetOccupant()->TakeChessPiece();

		Parent->GetCurrentBoardSquare()->Occupant = nullptr;
		DesiredMoveSquare->Occupant = Parent;
		Parent->CurrentBoardSquare = DesiredMoveSquare;
		Parent->bHasMoved = true;

		if(AChessGameMode* GameMode = Cast<AChessGameMode>(GetWorld()->GetAuthGameMode())) GameMode->EndTurn();
	}
	else{
		Server_MovePiece(DesiredMoveSquare);
	}
}

void UChessQueenBehaviour::TakePiece(ABoardSquare *DesiredMoveSquare)
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

void UChessQueenBehaviour::CanAttackSquare(ABoardSquare* TargetSquare, bool& bCanAttackSquare)
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

    if (const UWorld* World = GetWorld())
    {
        if (const AChessGameState* GameState = Cast<AChessGameState>(World->GetGameState<AGameStateBase>()))
        {
            TMap<FString, FMyMapContainer>& MyBoardData = GameState->GetChessBoard()->GetChessBoardData();
            FString HorizontalCoord = CurrentSquare->GetHorizontalCoordinate();
            int32 VerticalCoord = CurrentSquare->GetVerticalCoordinate();
            TCHAR HorizontalChar = HorizontalCoord[0];

            int32 TargetFile = TargetSquare->GetVerticalCoordinate();
            TCHAR TargetRank = TargetSquare->GetHorizontalCoordinate()[0];

            int32 FileDifference = FMath::Abs(TargetFile - VerticalCoord);
            int32 RankDifference = FMath::Abs(TargetRank - HorizontalChar);

            // Queen moves like both a rook (straight) or a bishop (diagonal)
            bool bIsDiagonal = FileDifference == RankDifference;
            bool bIsStraight = TargetRank == HorizontalChar || TargetFile == VerticalCoord;

            if (bIsDiagonal || bIsStraight)
            {
                int32 XDirection = bIsDiagonal ? (TargetRank > HorizontalChar ? 1 : -1) : (TargetRank == HorizontalChar ? 0 : (TargetRank > HorizontalChar ? 1 : -1));
                int32 YDirection = bIsDiagonal ? (TargetFile > VerticalCoord ? 1 : -1) : (TargetFile == VerticalCoord ? 0 : (TargetFile > VerticalCoord ? 1 : -1));

                int32 Steps = 1;
                while (Steps < FMath::Max(FileDifference, RankDifference))
                {
                    TCHAR NewHorizontal = HorizontalChar + XDirection * Steps;
                    int32 NewVertical = VerticalCoord + YDirection * Steps;

                    FString NewHorizontalCoord;
                    NewHorizontalCoord.AppendChar(NewHorizontal);

                    if (MyBoardData.Contains(NewHorizontalCoord) && MyBoardData[NewHorizontalCoord].SecondMap.Contains(NewVertical))
                    {
                        ABoardSquare* IntermediateSquare = MyBoardData[NewHorizontalCoord].SecondMap[NewVertical];

                        if (IntermediateSquare->GetOccupant())
                        {
                            bCanAttackSquare = false;
                            return;
                        }
                    }

                    if (NewHorizontal == TargetRank && NewVertical == TargetFile) break;
                    
                    Steps++;
                }

                // If the path is clear, check if the target square is occupied by an opponent
                if (TargetSquare->GetOccupant() && TargetSquare->GetOccupant()->GetChessTeam() != Parent->GetChessTeam())
                {
                    bCanAttackSquare = true;
                    return;
                }
                else
                {
                    bCanAttackSquare = true;
                    return;
                }
            }
        }
    }

    bCanAttackSquare = false;
}

void UChessQueenBehaviour::CalculateIsPinned(ABoardSquare *KingBoardSquare)
{
	// if is pinned
    if(ABaseChessPiece* Parent = Cast<ABaseChessPiece>(GetOwner()))
    {
        Parent->Attackers.Empty();

        if (UWorld* World = GetWorld())
        if (const AChessGameState* GameState = Cast<AChessGameState>(World->GetGameState<AGameStateBase>()))
        {
            ABoardSquare* CurrentSquare = Parent->GetCurrentBoardSquare();
            TArray<ABaseChessPiece*> EnemyPieces = Parent->GetChessTeam() == EChessTeam::White ? GameState->BlackPieces : GameState->WhitePieces;

            // What direction is the piece connected to the king itself?
            // From there check if enemy pieces are pinning the piece to king
            if(ChessHelperFunctions::IsDiagonalLineBetween(CurrentSquare, KingBoardSquare, World)) // Check queens and bishops
            {
                for(ABaseChessPiece* Piece : EnemyPieces)
                {
                    if(Piece->FindComponentByClass<UChessBishopBehaviour>() || Piece->FindComponentByClass<UChessQueenBehaviour>())
                    {
                        if(ChessHelperFunctions::IsDiagonalLineBetween(CurrentSquare, Piece->GetCurrentBoardSquare(), World))
                        {
                            Parent->bIsPinned = true;
                            Parent->PinDirection = EPinDirection::Diagonal;
                            Parent->Attackers.Add(Piece);
                            return;
                        }
                    }
                }
            }
            if(ChessHelperFunctions::IsHorizontalLineBetween(CurrentSquare, KingBoardSquare, World)) // Other two check for queens and rooks
            {
                for(ABaseChessPiece* Piece : EnemyPieces)
                {
                    if(Piece->FindComponentByClass<UChessQueenBehaviour>() || Piece->FindComponentByClass<UChessRookBehaviour>())
                    {
                        if(ChessHelperFunctions::IsHorizontalLineBetween(CurrentSquare, Piece->GetCurrentBoardSquare(), World))
                        {
                            Parent->bIsPinned = true;
                            Parent->PinDirection = EPinDirection::Horizontal;
                            Parent->Attackers.Add(Piece);
                            return;
                        }
                    }
                }
            }
            if(ChessHelperFunctions::IsVerticalLineBetween(CurrentSquare, KingBoardSquare, World))
            {
                for(ABaseChessPiece* Piece : EnemyPieces)
                {
                    if(Piece->FindComponentByClass<UChessQueenBehaviour>() || Piece->FindComponentByClass<UChessRookBehaviour>())
                    {
                        if(ChessHelperFunctions::IsVerticalLineBetween(CurrentSquare, Piece->GetCurrentBoardSquare(), World))
                        {
                            Parent->bIsPinned = true;
                            Parent->PinDirection = EPinDirection::Vertical;
                            Parent->Attackers.Add(Piece);
                            return;
                        }
                    }
                }
            }   
        }
        Parent->bIsPinned = false;
        Parent->PinDirection = EPinDirection::PinDirectionNone;
    }
}

void UChessQueenBehaviour::CalculateNumberOfValidMoves(int &numberOfValidMoves)
{
    numberOfValidMoves = 0;  // Initialize valid move count

    ABaseChessPiece* OwnerPiece = Cast<ABaseChessPiece>(GetOwner());
    ABoardSquare* CurrentBoardSquare = OwnerPiece->GetCurrentBoardSquare();

    if (!CurrentBoardSquare) return;

    if (UWorld* World = GetWorld())
    {
        if (const AChessGameState* GameState = Cast<AChessGameState>(World->GetGameState<AGameStateBase>()))
        {
            TMap<FString, FMyMapContainer>& MyBoardData = GameState->GetChessBoard()->GetChessBoardData();
            FString HorizontalCoord = CurrentBoardSquare->GetHorizontalCoordinate();
            int32 VerticalCoord = CurrentBoardSquare->GetVerticalCoordinate();
            TCHAR HorizontalChar = HorizontalCoord[0];

            ABaseChessPiece* King = (OwnerPiece->GetChessTeam() == EChessTeam::White) ? GameState->WhiteKing : GameState->BlackKing;

            // Queen combines both rook (straight) and bishop (diagonal) moves
            TArray<FIntPoint> QueenDirections = {
                {1, 0},  {-1, 0},   {0, 1},  {0, -1}, // Rook directions (horizontal/vertical)
                {1, 1},  {-1, 1},   {1, -1}, {-1, -1} // Bishop directions (diagonals)
            };

            // If the King is in check
            if (King->bInCheck)
            {
                if (OwnerPiece->PinDirection != EPinDirection::PinDirectionNone) return; // Can't move if pinned

                // Only allow moves that can block or capture the attacker
                ABaseChessPiece* Attacker = King->Attackers[0];

                for (const FIntPoint& Direction : QueenDirections)
                {
                    int32 Steps = 1;
                    while (true)
                    {
                        int32 NewVertical = VerticalCoord + (Direction.Y * Steps);
                        TCHAR NewHorizontal = HorizontalChar + (Direction.X * Steps);

                        // Check if the new position is within bounds ('A' to 'H' and 1 to 8)
                        if (NewHorizontal < 'A' || NewHorizontal > 'H' || NewVertical < 1 || NewVertical > 8)
                        {
                            break; // Out of bounds
                        }

                        FString NewHorizontalCoord;
                        NewHorizontalCoord.AppendChar(NewHorizontal);

                        if (MyBoardData.Contains(NewHorizontalCoord) && MyBoardData[NewHorizontalCoord].SecondMap.Contains(NewVertical))
                        {
                            ABoardSquare* TargetSquare = MyBoardData[NewHorizontalCoord].SecondMap[NewVertical];

                            // Can capture the attacker
                            if (TargetSquare == Attacker->GetCurrentBoardSquare())
                            {
                                numberOfValidMoves++;
                                if (numberOfValidMoves >= 1) return; // Stop as soon as a valid move is found
                                break; // Capture move valid, break to stop
                            }
                            // Can block the attack path if it's a straight or diagonal line
                            else if (ChessHelperFunctions::IsHorizontalLineBetween(TargetSquare, Attacker->GetCurrentBoardSquare(), World) 
                                     && ChessHelperFunctions::IsHorizontalLineBetween(TargetSquare, King->GetCurrentBoardSquare(), World))
                            {
                                numberOfValidMoves++;
                                if (numberOfValidMoves >= 1) return; // Stop as soon as a valid move is found
                            }
                            else if (ChessHelperFunctions::IsVerticalLineBetween(TargetSquare, Attacker->GetCurrentBoardSquare(), World)
                                     && ChessHelperFunctions::IsVerticalLineBetween(TargetSquare, King->GetCurrentBoardSquare(), World))
                            {
                                numberOfValidMoves++;
                                if (numberOfValidMoves >= 1) return; // Stop as soon as a valid move is found
                            }
                            else if (ChessHelperFunctions::IsDiagonalLineBetween(TargetSquare, Attacker->GetCurrentBoardSquare(), World) 
                                     && ChessHelperFunctions::IsDiagonalLineBetween(TargetSquare, King->GetCurrentBoardSquare(), World))
                            {
                                numberOfValidMoves++;
                                if (numberOfValidMoves >= 1) return; // Stop as soon as a valid move is found
                            }
                            else
                            {
                                break; // No valid block in this direction
                            }
                        }
                        Steps++;
                    }
                }
                return; // No further moves if in check
            }

            // If the Queen is pinned, restrict movement along the pin line
            if (OwnerPiece->bIsPinned)
            {
                for (const FIntPoint& Direction : QueenDirections)
                {
                    int32 Steps = 1;

                    while (true)
                    {
                        int32 NewVertical = VerticalCoord + (Direction.Y * Steps);
                        TCHAR NewHorizontal = HorizontalChar + (Direction.X * Steps);

                        // Check if the new position is within bounds
                        if (NewHorizontal < 'A' || NewHorizontal > 'H' || NewVertical < 1 || NewVertical > 8)
                        {
                            break; // Out of bounds
                        }

                        FString NewHorizontalCoord;
                        NewHorizontalCoord.AppendChar(NewHorizontal);

                        if (MyBoardData.Contains(NewHorizontalCoord) && MyBoardData[NewHorizontalCoord].SecondMap.Contains(NewVertical))
                        {
                            ABoardSquare* TargetSquare = MyBoardData[NewHorizontalCoord].SecondMap[NewVertical];

                            // Only allow moves along the pinning line (diagonal or straight)
                            if (ChessHelperFunctions::IsDiagonalLineBetween(OwnerPiece->Attackers[0]->GetCurrentBoardSquare(), TargetSquare, World) ||
                                ChessHelperFunctions::IsHorizontalLineBetween(OwnerPiece->Attackers[0]->GetCurrentBoardSquare(), TargetSquare, World) ||
                                ChessHelperFunctions::IsVerticalLineBetween(OwnerPiece->Attackers[0]->GetCurrentBoardSquare(), TargetSquare, World))
                            {
                                if (!TargetSquare->GetOccupant())
                                {
                                    numberOfValidMoves++;
                                    if (numberOfValidMoves >= 1) return; // Stop as soon as a valid move is found
                                }
                                else if (TargetSquare == OwnerPiece->Attackers[0]->GetCurrentBoardSquare())
                                {
                                    numberOfValidMoves++;
                                    if (numberOfValidMoves >= 1) return; // Stop as soon as a valid move is found
                                    break; // Capture pinning piece
                                }
                                else
                                {
                                    break; // Blocked by own piece
                                }
                            }
                            else
                            {
                                break; // Invalid move outside the pin line
                            }
                        }
                        Steps++;
                    }
                }
                return; // Can't move further if pinned
            }

            // Default Queen movement (not in check or pinned)
            for (const FIntPoint& Direction : QueenDirections)
            {
                int32 Steps = 1;

                while (true)
                {
                    int32 NewVertical = VerticalCoord + (Direction.Y * Steps);
                    TCHAR NewHorizontal = HorizontalChar + (Direction.X * Steps);

                    // Check if the new position is within bounds
                    if (NewHorizontal < 'A' || NewHorizontal > 'H' || NewVertical < 1 || NewVertical > 8)
                    {
                        break; // Out of bounds
                    }

                    FString NewHorizontalCoord;
                    NewHorizontalCoord.AppendChar(NewHorizontal);

                    if (MyBoardData.Contains(NewHorizontalCoord) && MyBoardData[NewHorizontalCoord].SecondMap.Contains(NewVertical))
                    {
                        ABoardSquare* TargetSquare = MyBoardData[NewHorizontalCoord].SecondMap[NewVertical];

                        // Empty square is a valid move
                        if (!TargetSquare->GetOccupant())
                        {
                            numberOfValidMoves++;
                            if (numberOfValidMoves >= 1) return; // Stop as soon as a valid move is found
                            Steps++; // Continue in the same direction
                        }
                        else
                        {
                            // Occupied by opponent's piece
                            if (TargetSquare->GetOccupant()->GetChessTeam() != OwnerPiece->GetChessTeam())
                            {
                                numberOfValidMoves++;
                                if (numberOfValidMoves >= 1) return; // Stop as soon as a valid move is found
                            }
                            break; // Stop after finding a piece
                        }
                    }
                    else
                    {
                        break; // No square found, stop this direction
                    }
                }
            }
        }
    }
}

void UChessQueenBehaviour::Server_MovePiece_Implementation(ABoardSquare *DesiredMoveSquare)
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
	Parent->bHasMoved = true;

	if(AChessGameMode* GameMode = Cast<AChessGameMode>(GetWorld()->GetAuthGameMode())) GameMode->EndTurn();
}

