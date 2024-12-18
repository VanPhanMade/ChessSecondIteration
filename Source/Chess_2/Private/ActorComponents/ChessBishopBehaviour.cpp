// Copyright (©) 2024, Van Phan. All rights reserved.


#include "ActorComponents/ChessBishopBehaviour.h"
#include "Actors/BaseChessPiece.h"
#include "GameStates/ChessGameState.h"
#include "Actors/ChessBoard.h"
#include "Actors/BoardSquare.h"
#include "GameModes/ChessGameMode.h"
#include "ActorComponents/ChessBishopBehaviour.h"
#include "ActorComponents/ChessQueenBehaviour.h"
#include "ActorComponents/ChessRookBehaviour.h"
#include "Utility/ChessHelperFunctions.h"

UChessBishopBehaviour::UChessBishopBehaviour()
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

void UChessBishopBehaviour::CalculateValidMoves(bool &bInCheck, bool &bIsPinned, bool &bIsProtected, bool &bHasMoved, ABoardSquare *CurrentBoardSquare)
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

            // Bishop moves in four diagonal directions
            TArray<FIntPoint> BishopDirections = {
                {1, 1}, {-1, 1},   // Top-right and top-left
                {1, -1}, {-1, -1}  // Bottom-right and bottom-left
            };

            // If the King is in check
            if (King->bInCheck)
            {
                if (bIsPinned) return; // Can't move if pinned

                // Only allow moves that can block or capture the attacker
                ABaseChessPiece* Attacker = King->Attackers[0];

                for (const FIntPoint& Direction : BishopDirections)
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

                        FString NewHorizontalCoord;
                        NewHorizontalCoord.AppendChar(NewHorizontal);

                        if (MyBoardData.Contains(NewHorizontalCoord) && MyBoardData[NewHorizontalCoord].SecondMap.Contains(NewVertical))
                        {
                            ABoardSquare* TargetSquare = MyBoardData[NewHorizontalCoord].SecondMap[NewVertical];

                            // Can move to block the attack path
                            if (TargetSquare == Attacker->GetCurrentBoardSquare())
                            {
                                // Bishop can capture the attacker
                                TargetSquare->ShowValidMoves(true);
                                TargetSquare->HighlightSquare(true);
                                break;
                            }
                            else if (ChessHelperFunctions::IsDiagonalLineBetween(TargetSquare, Attacker->GetCurrentBoardSquare(), World))
							{
								TargetSquare->ShowValidMoves(true); // Bishop can block the path
							}
                            else
                            {
                                break; // No further moves in this direction
                            }
                        }
                        Steps++;
                    }
                }
                return;
            }

            // If the Bishop is pinned, restrict movement
            if (bIsPinned)
            {
                for (const FIntPoint& Direction : BishopDirections)
                {
                    int32 Steps = 1;

                    while (true)
                    {
                        int32 NewVertical = VerticalCoord + (Direction.Y * Steps);
                        TCHAR NewHorizontal = HorizontalChar + (Direction.X * Steps);

                        // Check if the new position is within bounds
                        if (NewHorizontal < 'A' || NewHorizontal > 'H' || NewVertical < 1 || NewVertical > 8)
                        {
                            break; // Out of bounds, stop searching this direction
                        }

                        FString NewHorizontalCoord;
                        NewHorizontalCoord.AppendChar(NewHorizontal);

                        if (MyBoardData.Contains(NewHorizontalCoord) && MyBoardData[NewHorizontalCoord].SecondMap.Contains(NewVertical))
                        {
                            ABoardSquare* TargetSquare = MyBoardData[NewHorizontalCoord].SecondMap[NewVertical];

                            // Bishop can only move along the line of the pin
							if (ChessHelperFunctions::IsDiagonalLineBetween(OwnerPiece->Attackers[0]->CurrentBoardSquare, TargetSquare, World))
							{
								if (!TargetSquare->GetOccupant())
								{
									TargetSquare->ShowValidMoves(true); // Empty square
								}
								else if (TargetSquare == OwnerPiece->Attackers[0]->CurrentBoardSquare)
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
								break; // Invalid move outside the pinning line
							}
                        }
                        Steps++;
                    }
                }
                return;
            }

            // Default unrestricted movement (not in check or pinned)
            for (const FIntPoint& Direction : BishopDirections)
            {
                int32 Steps = 1;

                while (true)
                {
                    int32 NewVertical = VerticalCoord + (Direction.Y * Steps);
                    TCHAR NewHorizontal = HorizontalChar + (Direction.X * Steps);

                    // Check if the new position is within bounds
                    if (NewHorizontal < 'A' || NewHorizontal > 'H' || NewVertical < 1 || NewVertical > 8)
                    {
                        break; // Out of bounds, stop searching this direction
                    }

                    FString NewHorizontalCoord;
                    NewHorizontalCoord.AppendChar(NewHorizontal);

                    if (MyBoardData.Contains(NewHorizontalCoord) && MyBoardData[NewHorizontalCoord].SecondMap.Contains(NewVertical))
                    {
                        ABoardSquare* TargetSquare = MyBoardData[NewHorizontalCoord].SecondMap[NewVertical];

                        // If the square is empty, it's a valid move
                        if (!TargetSquare->GetOccupant())
                        {
                            TargetSquare->ShowValidMoves(true);
                            Steps++; // Continue in the same direction
                        }
                        else
                        {
                            // If the square contains an opponent's piece, it's a valid move
                            if (TargetSquare->GetOccupant()->GetChessTeam() != OwnerPiece->GetChessTeam())
                            {
                                TargetSquare->ShowValidMoves(true);
                                TargetSquare->HighlightSquare(true);
                            }
                            break; // Stop after encountering any piece
                        }
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

void UChessBishopBehaviour::HideMoves(ABoardSquare *CurrentBoardSquare)
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

			// Bishop moves in four diagonal directions: top-right, top-left, bottom-right, bottom-left
			TArray<FIntPoint> BishopDirections = {
				{1, 1},  {-1, 1},   // Top-right and top-left
				{1, -1}, {-1, -1}   // Bottom-right and bottom-left
			};

			for (const FIntPoint& Direction : BishopDirections)
			{
				int32 Steps = 1;

				while(true)
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
						Steps++; // Continue in the same direction
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

void UChessBishopBehaviour::MovePiece(ABoardSquare *DesiredMoveSquare)
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

		if(AChessGameMode* GameMode = Cast<AChessGameMode>(GetWorld()->GetAuthGameMode()))
		{
			GameMode->EndTurn();
		}
	}
	else{
		Server_MovePiece(DesiredMoveSquare);
	}
}

void UChessBishopBehaviour::TakePiece(ABoardSquare *DesiredMoveSquare)
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

void UChessBishopBehaviour::CanAttackSquare(ABoardSquare* TargetSquare, bool& bCanAttackSquare)
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
            int32 VerticalCoord = CurrentSquare->GetVerticalCoordinate();
            TCHAR HorizontalChar = CurrentSquare->GetHorizontalCoordinate()[0];
			// UE_LOG(LogTemp, Display, TEXT("Current Square: Horizontal: %c, Vertical: %d"), HorizontalChar, VerticalCoord);

            int32 TargetFile = TargetSquare->GetVerticalCoordinate();
            TCHAR TargetRank = TargetSquare->GetHorizontalCoordinate()[0];

            int32 FileDifference = FMath::Abs(TargetFile - VerticalCoord);
            int32 RankDifference = FMath::Abs(TargetRank - HorizontalChar);

            // Bishop moves diagonally, so the difference in file and rank must be the same
            if (FileDifference == RankDifference)
            {
                // Determine the direction to move in (1 or -1 for each axis)
                int32 XDirection = (TargetRank > HorizontalChar) ? 1 : -1;
                int32 YDirection = (TargetFile > VerticalCoord) ? 1 : -1;

                int32 Steps = 1;
                while (Steps < FileDifference)
                {
                    TCHAR NewHorizontal = HorizontalChar + XDirection * Steps;
                    int32 NewVertical = VerticalCoord + YDirection * Steps;

                    FString NewHorizontalCoord;
                    NewHorizontalCoord.AppendChar(NewHorizontal);

                    if (MyBoardData.Contains(NewHorizontalCoord) && MyBoardData[NewHorizontalCoord].SecondMap.Contains(NewVertical))
                    {
                        ABoardSquare* IntermediateSquare = MyBoardData[NewHorizontalCoord].SecondMap[NewVertical];
						// UE_LOG(LogTemp, Display, TEXT("Checking square: %s, %d"), *NewHorizontalCoord, NewVertical);
                        // If any square between the current and target square has an occupant, stop the movement
                        if (IntermediateSquare->GetOccupant())
                        {
                            bCanAttackSquare = false;
                            return;
                        }
                    }

                    if (NewHorizontal == TargetRank && NewVertical == TargetFile) break;
                    
                    Steps++;
                }

                // If the target square is reachable, check if it's occupied by an opponent
                if (TargetSquare->GetOccupant() && TargetSquare->GetOccupant()->GetChessTeam() != Parent->GetChessTeam())
                {
					// UE_LOG(LogTemp, Display, TEXT("Bishop can attack: %s"), *TargetSquare->GetOccupant()->GetName());
                    bCanAttackSquare = true;
                    return;
                }
                else
                {
                    // UE_LOG(LogTemp, Display, TEXT("Bishop can attack empty square. "));
                    bCanAttackSquare = true;
                    return;
                }
            }
        }
    }
    bCanAttackSquare = false;
}

void UChessBishopBehaviour::CalculateIsPinned(ABoardSquare *KingBoardSquare)
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

void UChessBishopBehaviour::CalculateNumberOfValidMoves(int &numberOfValidMoves)
{
    numberOfValidMoves = 0;  // Initialize valid move count

    // Get the board square the bishop is currently on
    ABaseChessPiece* OwnerPiece = Cast<ABaseChessPiece>(GetOwner());
    ABoardSquare* CurrentBoardSquare = OwnerPiece->GetCurrentBoardSquare();

    if (!CurrentBoardSquare) return;

    if (UWorld* World = GetWorld())
    {
        if (const AChessGameState* GameState = Cast<AChessGameState>(World->GetGameState<AGameStateBase>()))
        {
            // Get current board coordinates
            TMap<FString, FMyMapContainer>& MyBoardData = GameState->GetChessBoard()->GetChessBoardData();
            FString HorizontalCoord = CurrentBoardSquare->GetHorizontalCoordinate();
            int32 VerticalCoord = CurrentBoardSquare->GetVerticalCoordinate();
            TCHAR HorizontalChar = HorizontalCoord[0];

            ABaseChessPiece* King = (OwnerPiece->GetChessTeam() == EChessTeam::White) ? GameState->WhiteKing : GameState->BlackKing;

            // Bishop moves in four diagonal directions
            TArray<FIntPoint> BishopDirections = {
                {1, 1}, {-1, 1},   // Top-right and top-left
                {1, -1}, {-1, -1}  // Bottom-right and bottom-left
            };

            bool bIsPinned = OwnerPiece->bIsPinned;

            // 1. Case: King is in check and bishop is not pinned
            if (King->bInCheck && !bIsPinned)
            {
                ABaseChessPiece* Attacker = King->Attackers[0];

                for (const FIntPoint& Direction : BishopDirections)
                {
                    int32 Steps = 1;
                    while (true)
                    {
                        int32 NewVertical = VerticalCoord + (Direction.Y * Steps);
                        TCHAR NewHorizontal = HorizontalChar + (Direction.X * Steps);

                        // Check if the new position is within bounds
                        if (NewHorizontal < 'A' || NewHorizontal > 'H' || NewVertical < 1 || NewVertical > 8)
                        {
                            break;  // Out of bounds, stop searching in this direction
                        }

                        FString NewHorizontalCoord;
                        NewHorizontalCoord.AppendChar(NewHorizontal);

                        if (MyBoardData.Contains(NewHorizontalCoord) && MyBoardData[NewHorizontalCoord].SecondMap.Contains(NewVertical))
                        {
                            ABoardSquare* TargetSquare = MyBoardData[NewHorizontalCoord].SecondMap[NewVertical];

                            // If the bishop can block the attack path or capture the attacker
                            if (TargetSquare == Attacker->GetCurrentBoardSquare() ||
                                ChessHelperFunctions::IsDiagonalLineBetween(TargetSquare, Attacker->GetCurrentBoardSquare(), World))
                            {
                                numberOfValidMoves++;
                                if (numberOfValidMoves >= 1) return;  // Stop as soon as a valid move is found
                            }

                            // Stop if the bishop encounters any piece
                            if (TargetSquare->GetOccupant())
                            {
                                break;
                            }
                        }
                        else
                        {
                            break;  // No square found, stop searching in this direction
                        }
                        Steps++;
                    }
                }
                return;  // Stop after calculating valid moves to block or capture the attacker
            }

            // 2. Case: King is in check and bishop is pinned
            if (King->bInCheck && bIsPinned)
            {
                return;  // Bishop cannot move when the king is in check and it is pinned
            }

            // 3. Case: Bishop is pinned but the king is not in check
            if (bIsPinned)
            {
                for (const FIntPoint& Direction : BishopDirections)
                {
                    int32 Steps = 1;

                    while (true)
                    {
                        int32 NewVertical = VerticalCoord + (Direction.Y * Steps);
                        TCHAR NewHorizontal = HorizontalChar + (Direction.X * Steps);

                        // Check if the new position is within bounds
                        if (NewHorizontal < 'A' || NewHorizontal > 'H' || NewVertical < 1 || NewVertical > 8)
                        {
                            break;  // Out of bounds, stop searching in this direction
                        }

                        FString NewHorizontalCoord;
                        NewHorizontalCoord.AppendChar(NewHorizontal);

                        if (MyBoardData.Contains(NewHorizontalCoord) && MyBoardData[NewHorizontalCoord].SecondMap.Contains(NewVertical))
                        {
                            ABoardSquare* TargetSquare = MyBoardData[NewHorizontalCoord].SecondMap[NewVertical];

                            // If the bishop is pinned, it can only move along the pinning line
                            if (ChessHelperFunctions::IsDiagonalLineBetween(OwnerPiece->Attackers[0]->GetCurrentBoardSquare(), TargetSquare, World))
                            {
                                // If the square is empty or contains an opponent's piece, it is a valid move
                                if (!TargetSquare->GetOccupant() || TargetSquare->GetOccupant()->GetChessTeam() != OwnerPiece->GetChessTeam())
                                {
                                    numberOfValidMoves++;
                                    if (numberOfValidMoves >= 1) return;  // Stop as soon as a valid move is found
                                }

                                // Stop if the bishop encounters any piece
                                if (TargetSquare->GetOccupant())
                                {
                                    break;
                                }
                            }
                            else
                            {
                                break;  // Invalid move outside the pinning line
                            }
                        }
                        Steps++;
                    }
                }
                return;  // Stop after calculating all valid moves along the pin line
            }

            // 4. Case: Unrestricted movement (not pinned or in check)
            for (const FIntPoint& Direction : BishopDirections)
            {
                int32 Steps = 1;
                while (true)
                {
                    int32 NewVertical = VerticalCoord + (Direction.Y * Steps);
                    TCHAR NewHorizontal = HorizontalChar + (Direction.X * Steps);

                    // Check if the new position is within bounds
                    if (NewHorizontal < 'A' || NewHorizontal > 'H' || NewVertical < 1 || NewVertical > 8)
                    {
                        break;  // Out of bounds, stop searching in this direction
                    }

                    FString NewHorizontalCoord;
                    NewHorizontalCoord.AppendChar(NewHorizontal);

                    if (MyBoardData.Contains(NewHorizontalCoord) && MyBoardData[NewHorizontalCoord].SecondMap.Contains(NewVertical))
                    {
                        ABoardSquare* TargetSquare = MyBoardData[NewHorizontalCoord].SecondMap[NewVertical];

                        // If the square is empty or contains an opponent's piece, it is a valid move
                        if (!TargetSquare->GetOccupant() || TargetSquare->GetOccupant()->GetChessTeam() != OwnerPiece->GetChessTeam())
                        {
                            numberOfValidMoves++;
                            if (numberOfValidMoves >= 1) return;  // Stop as soon as a valid move is found
                        }

                        // Stop if the bishop encounters any piece
                        if (TargetSquare->GetOccupant())
                        {
                            break;
                        }
                    }
                    else
                    {
                        break;  // No square found, stop searching in this direction
                    }
                }
            }
        }
    }
}

void UChessBishopBehaviour::Server_MovePiece_Implementation(ABoardSquare *DesiredMoveSquare)
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

	if(AChessGameMode* GameMode = Cast<AChessGameMode>(GetWorld()->GetAuthGameMode()))  GameMode->EndTurn();


}

