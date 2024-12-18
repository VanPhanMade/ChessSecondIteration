// Copyright (©) 2024, Van Phan. All rights reserved.


#include "ActorComponents/ChessRookBehaviour.h"
#include "Actors/BaseChessPiece.h"
#include "GameStates/ChessGameState.h"
#include "Actors/ChessBoard.h"
#include "Actors/BoardSquare.h"
#include "GameModes/ChessGameMode.h"
#include "ActorComponents/ChessBishopBehaviour.h"
#include "ActorComponents/ChessQueenBehaviour.h"
#include "ActorComponents/ChessRookBehaviour.h"
#include "Utility/ChessHelperFunctions.h"

UChessRookBehaviour::UChessRookBehaviour()
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

void UChessRookBehaviour::CalculateValidMoves(bool &bInCheck, bool &bIsPinned, bool &bIsProtected, bool &bHasMoved, ABoardSquare *CurrentBoardSquare)
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

            // Rook moves in four straight directions: left, right, up, down
            TArray<FIntPoint> RookDirections = {
                {1, 0},  {-1, 0},   // Horizontal: right, left
                {0, 1},  {0, -1}    // Vertical: up, down
            };

            // Handle case when the King is in check
            if (King->bInCheck)
            {
                if (bIsPinned) return; // Rook cannot move if pinned

                // Only allow moves that can block or capture the attacker
                ABaseChessPiece* Attacker = King->Attackers[0];

                for (const FIntPoint& Direction : RookDirections)
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

                            // Can move to capture the attacker
                            if (TargetSquare == Attacker->GetCurrentBoardSquare())
                            {
                                TargetSquare->ShowValidMoves(true);
                                TargetSquare->HighlightSquare(true);
                                break;
                            }
                            // Check if a straight line is obstructed between the king and attacker
                            else if (ChessHelperFunctions::IsHorizontalLineBetween(TargetSquare, Attacker->GetCurrentBoardSquare(), World) && ChessHelperFunctions::IsHorizontalLineBetween(TargetSquare, King->GetCurrentBoardSquare(), World))
                            {
                                TargetSquare->ShowValidMoves(true);
								break;
                            }
							else if (ChessHelperFunctions::IsDiagonalLineBetween(TargetSquare, Attacker->GetCurrentBoardSquare(), World) && ChessHelperFunctions::IsDiagonalLineBetween(TargetSquare, King->GetCurrentBoardSquare(), World))
                            {
                                TargetSquare->ShowValidMoves(true);
								break;
                            }
							else if (ChessHelperFunctions::IsVerticalLineBetween(TargetSquare, Attacker->GetCurrentBoardSquare(), World) && ChessHelperFunctions::IsVerticalLineBetween(TargetSquare, King->GetCurrentBoardSquare(), World))
                            {
                                TargetSquare->ShowValidMoves(true);
								break;
                            }
                        }
						else
						{
							break;
						}
                        Steps++;
                    }
                }
                return;
            }

            // Handle case when the Rook is pinned
            if (bIsPinned)
            {
                for (const FIntPoint& Direction : RookDirections)
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

                            // Rook can only move along the line of the pin (horizontal or vertical)
                            if (OwnerPiece->PinDirection == Horizontal ? ChessHelperFunctions::IsHorizontalLineBetween(OwnerPiece->Attackers[0]->GetCurrentBoardSquare(), TargetSquare, World) :
							OwnerPiece->PinDirection == Vertical ? ChessHelperFunctions::IsVerticalLineBetween(OwnerPiece->Attackers[0]->GetCurrentBoardSquare(), TargetSquare, World) : false
							)
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
                                break; // Invalid move outside the pinning line
                            }
                        }
                        Steps++;
                    }
                }
                return;
            }

            // Default movement (not in check or pinned)
            for (const FIntPoint& Direction : RookDirections)
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
                            break; // Stop after encountering a piece
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

void UChessRookBehaviour::HideMoves(ABoardSquare *CurrentBoardSquare)
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

			// Rook moves in four straight directions: left, right, up, down
			TArray<FIntPoint> RookDirections = {
				{1, 0},  {-1, 0},   // Horizontal: right, left
				{0, 1},  {0, -1}    // Vertical: up, down
			};

			for (const FIntPoint& Direction : RookDirections)
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

void UChessRookBehaviour::MovePiece(ABoardSquare *DesiredMoveSquare)
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

void UChessRookBehaviour::TakePiece(ABoardSquare *DesiredMoveSquare)
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

void UChessRookBehaviour::CanAttackSquare(ABoardSquare* TargetSquare, bool& bCanAttackSquare)
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

            // Rook moves in straight lines (either horizontal or vertical)
            if (TargetRank == HorizontalChar || TargetFile == VerticalCoord)
            {
                int32 XDirection = (TargetRank == HorizontalChar) ? 0 : (TargetRank > HorizontalChar ? 1 : -1);
                int32 YDirection = (TargetFile == VerticalCoord) ? 0 : (TargetFile > VerticalCoord ? 1 : -1);

                int32 Steps = 1;
                while (true)
                {
                    TCHAR NewHorizontal = HorizontalChar + XDirection * Steps;
                    int32 NewVertical = VerticalCoord + YDirection * Steps;

                    if (NewHorizontal < 'A' || NewHorizontal > 'H' || NewVertical < 1 || NewVertical > 8) break;

                    FString NewHorizontalCoord;
                    NewHorizontalCoord.AppendChar(NewHorizontal);

                    if (MyBoardData.Contains(NewHorizontalCoord) && MyBoardData[NewHorizontalCoord].SecondMap.Contains(NewVertical))
                    {
                        ABoardSquare* IntermediateSquare = MyBoardData[NewHorizontalCoord].SecondMap[NewVertical];

                        // Stop if any square between the current and target square is occupied
                        if (IntermediateSquare->GetOccupant())
                        {
                            bCanAttackSquare = false;
                            return;
                        }
                    }

                    if (NewHorizontal == TargetRank && NewVertical == TargetFile) break;

                    Steps++;
                }

                // Check if the target square is occupied by an opponent piece
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

void UChessRookBehaviour::CalculateIsPinned(ABoardSquare *KingBoardSquare)
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

void UChessRookBehaviour::CalculateNumberOfValidMoves(int &numberOfValidMoves)
{
    numberOfValidMoves = 0; // Initialize the count of valid moves to zero.

    if (UWorld* World = GetWorld())
    {
        if (const AChessGameState* GameState = Cast<AChessGameState>(World->GetGameState<AGameStateBase>()))
        {
            TMap<FString, FMyMapContainer>& MyBoardData = GameState->GetChessBoard()->GetChessBoardData();
            ABaseChessPiece* OwnerPiece = Cast<ABaseChessPiece>(GetOwner());
            ABaseChessPiece* King = (OwnerPiece->GetChessTeam() == EChessTeam::White) ? GameState->WhiteKing : GameState->BlackKing;

            // Rook moves in four straight directions: left, right, up, down
            TArray<FIntPoint> RookDirections = {
                {1, 0},  {-1, 0},   // Horizontal: right, left
                {0, 1},  {0, -1}    // Vertical: up, down
            };

            // Handle case when the King is in check
            if (King->bInCheck)
            {
                if (OwnerPiece->bIsPinned) return; // Rook cannot move if pinned

                // Only allow moves that can block or capture the attacker
                ABaseChessPiece* Attacker = King->Attackers[0];

                for (const FIntPoint& Direction : RookDirections)
                {
                    int32 Steps = 1;
                    while (true)
                    {
                        // Determine the new coordinates based on direction and steps
                        int32 NewVertical = OwnerPiece->GetCurrentBoardSquare()->GetVerticalCoordinate() + (Direction.Y * Steps);
                        TCHAR NewHorizontal = OwnerPiece->GetCurrentBoardSquare()->GetHorizontalCoordinate()[0] + (Direction.X * Steps);

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

                            // Can move to capture the attacker
                            if (TargetSquare == Attacker->GetCurrentBoardSquare())
                            {
                                numberOfValidMoves++; // Count the valid capture
                                break;
                            }

                            // Check if a straight line is obstructed between the king and attacker
                            if (ChessHelperFunctions::IsHorizontalLineBetween(TargetSquare, Attacker->GetCurrentBoardSquare(), World) && 
                                ChessHelperFunctions::IsHorizontalLineBetween(TargetSquare, King->GetCurrentBoardSquare(), World))
                            {
                                numberOfValidMoves++;
                                break;
                            }
                            else if (ChessHelperFunctions::IsVerticalLineBetween(TargetSquare, Attacker->GetCurrentBoardSquare(), World) && 
                                     ChessHelperFunctions::IsVerticalLineBetween(TargetSquare, King->GetCurrentBoardSquare(), World))
                            {
                                numberOfValidMoves++;
                                break;
                            }
                        }
                        else
                        {
                            break; // Out of bounds
                        }
                        Steps++;
                    }
                }
                return;
            }

            // Handle case when the Rook is pinned
            if (OwnerPiece->bIsPinned)
            {
                for (const FIntPoint& Direction : RookDirections)
                {
                    int32 Steps = 1;

                    while (true)
                    {
                        // Determine the new coordinates based on direction and steps
                        int32 NewVertical = OwnerPiece->GetCurrentBoardSquare()->GetVerticalCoordinate() + (Direction.Y * Steps);
                        TCHAR NewHorizontal = OwnerPiece->GetCurrentBoardSquare()->GetHorizontalCoordinate()[0] + (Direction.X * Steps);

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

                            // Rook can only move along the line of the pin (horizontal or vertical)
                            if (OwnerPiece->PinDirection == EPinDirection::Horizontal && 
                                ChessHelperFunctions::IsHorizontalLineBetween(OwnerPiece->Attackers[0]->GetCurrentBoardSquare(), TargetSquare, World))
                            {
                                if (!TargetSquare->GetOccupant())
                                {
                                    numberOfValidMoves++; // Empty square
                                }
                                else if (TargetSquare == OwnerPiece->Attackers[0]->GetCurrentBoardSquare())
                                {
                                    numberOfValidMoves++; // Capture pinning piece
                                    break;
                                }
                                else
                                {
                                    break; // Blocked by own piece
                                }
                            }
                            else if (OwnerPiece->PinDirection == EPinDirection::Vertical && 
                                     ChessHelperFunctions::IsVerticalLineBetween(OwnerPiece->Attackers[0]->GetCurrentBoardSquare(), TargetSquare, World))
                            {
                                if (!TargetSquare->GetOccupant())
                                {
                                    numberOfValidMoves++; // Empty square
                                }
                                else if (TargetSquare == OwnerPiece->Attackers[0]->GetCurrentBoardSquare())
                                {
                                    numberOfValidMoves++; // Capture pinning piece
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

            // Default movement (not in check or pinned)
            for (const FIntPoint& Direction : RookDirections)
            {
                int32 Steps = 1;

                while (true)
                {
                    // Determine the new coordinates based on direction and steps
                    int32 NewVertical = OwnerPiece->GetCurrentBoardSquare()->GetVerticalCoordinate() + (Direction.Y * Steps);
                    TCHAR NewHorizontal = OwnerPiece->GetCurrentBoardSquare()->GetHorizontalCoordinate()[0] + (Direction.X * Steps);

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

                        // If the square is empty, it's a valid move
                        if (!TargetSquare->GetOccupant())
                        {
                            numberOfValidMoves++;
                            Steps++; // Continue in the same direction
                        }
                        else
                        {
                            // If the square contains an opponent's piece, it's a valid move
                            if (TargetSquare->GetOccupant()->GetChessTeam() != OwnerPiece->GetChessTeam())
                            {
                                numberOfValidMoves++;
                            }
                            break; // Stop after encountering a piece
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

void UChessRookBehaviour::Server_MovePiece_Implementation(ABoardSquare *DesiredMoveSquare)
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

