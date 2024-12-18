// Copyright (©) 2024, Van Phan. All rights reserved.


#include "ActorComponents/ChessKnightBehaviour.h"
#include "Actors/BaseChessPiece.h"
#include "GameStates/ChessGameState.h"
#include "Actors/ChessBoard.h"
#include "Actors/BoardSquare.h"
#include "GameModes/ChessGameMode.h"
#include "ActorComponents/ChessBishopBehaviour.h"
#include "ActorComponents/ChessQueenBehaviour.h"
#include "ActorComponents/ChessRookBehaviour.h"
#include "Utility/ChessHelperFunctions.h"

UChessKnightBehaviour::UChessKnightBehaviour()
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

void UChessKnightBehaviour::CalculateValidMoves(bool &bInCheck, bool &bIsPinned, bool &bIsProtected, bool &bHasMoved, ABoardSquare *CurrentBoardSquare)
{
	if (!CurrentBoardSquare) return;

	if(UWorld* World = GetWorld())
	{
		if(const AChessGameState* GameState = Cast<AChessGameState>(World->GetGameState<AGameStateBase>()))
		{
			TMap<FString, FMyMapContainer>& MyBoardData = GameState->GetChessBoard()->GetChessBoardData();
			FString HorizontalCoord = CurrentBoardSquare->GetHorizontalCoordinate();
			int32 VerticalCoord = CurrentBoardSquare->GetVerticalCoordinate();
			TCHAR HorizontalChar = HorizontalCoord[0];
			MyBoardData[HorizontalCoord].SecondMap[VerticalCoord]->HighlightSquare(false);

			ABaseChessPiece* OwnerPiece = Cast<ABaseChessPiece>(GetOwner());
			ABaseChessPiece* King = (OwnerPiece->GetChessTeam() == EChessTeam::White) ? GameState->WhiteKing : GameState->BlackKing;
			
			TArray<FIntPoint> KnightMoves = {
				{2, 1}, {2, -1}, {-2, 1}, {-2, -1},
				{1, 2}, {1, -2}, {-1, 2}, {-1, -2}
			};

			// If the King is in check, restrict moves to blocking or taking the attacker
			if (King->bInCheck)
			{
				// Cannot move if pinned
				if (bIsPinned) return;

				// Only valid if the Knight can block or capture the attacker
				if (King->Attackers.Num() == 1)
				{
					ABaseChessPiece* Attacker = King->Attackers[0];

					for (const FIntPoint& Move : KnightMoves)
					{
						TCHAR NewHorizontal = HorizontalChar + Move.X;
						int32 NewVertical = VerticalCoord + Move.Y;

						// Check if the new position is within bounds ('A' to 'H' and 1 to 8)
						if (NewHorizontal >= 'A' && NewHorizontal <= 'H' && NewVertical >= 1 && NewVertical <= 8)
						{
							FString NewHorizontalCoord;
							NewHorizontalCoord.AppendChar(NewHorizontal);

							// Validate move: can only move to the attacker's square or block the attack path
							if (MyBoardData.Contains(NewHorizontalCoord) && MyBoardData[NewHorizontalCoord].SecondMap.Contains(NewVertical))
							{
								ABoardSquare* TargetSquare = MyBoardData[NewHorizontalCoord].SecondMap[NewVertical];
								
								// Can move to attack the attacker or block the attack path
								if (TargetSquare == Attacker->GetCurrentBoardSquare())
								{
									// If the knight can capture the attacker directly
									TargetSquare->ShowValidMoves(true);
									TargetSquare->HighlightSquare(true);
								}
								else
								{
									// Check if the knight can block the attack path
									int HorizontalDifference = FMath::Abs(King->GetCurrentBoardSquare()->GetHorizontalCoordinate()[0] - Attacker->GetCurrentBoardSquare()->GetHorizontalCoordinate()[0]);
									int VerticalDifference = FMath::Abs(King->GetCurrentBoardSquare()->GetVerticalCoordinate() - Attacker->GetCurrentBoardSquare()->GetVerticalCoordinate());

									// Diagonal Attack
									if (HorizontalDifference == VerticalDifference && ChessHelperFunctions::IsDiagonalLineBetween(Attacker->GetCurrentBoardSquare(), TargetSquare, World)
									&& ChessHelperFunctions::IsDiagonalLineBetween(King->GetCurrentBoardSquare(), TargetSquare, World))
									{
										TargetSquare->ShowValidMoves(true);
									}
									// Vertical Attack
									else if (HorizontalDifference == 0 && ChessHelperFunctions::IsVerticalLineBetween(Attacker->GetCurrentBoardSquare(), TargetSquare, World)
									&& ChessHelperFunctions::IsVerticalLineBetween(King->GetCurrentBoardSquare(), TargetSquare, World))
									{
										TargetSquare->ShowValidMoves(true);
									}
									// Horizontal Attack
									else if (VerticalDifference == 0 && ChessHelperFunctions::IsHorizontalLineBetween(Attacker->GetCurrentBoardSquare(), TargetSquare, World)
									&& ChessHelperFunctions::IsHorizontalLineBetween(King->GetCurrentBoardSquare(), TargetSquare, World))
									{
										TargetSquare->ShowValidMoves(true);
									}
								}
							}
						}
					}
				}
				return;
			}

			// If the Knight is pinned, restrict movement
			if (bIsPinned)
			{
				EPinDirection PinDirection = OwnerPiece->PinDirection;

				// Knights cannot break a pin unless they capture the pinning piece
				for (const FIntPoint& Move : KnightMoves)
				{
					TCHAR NewHorizontal = HorizontalChar + Move.X;
					int32 NewVertical = VerticalCoord + Move.Y;

					if (NewHorizontal >= 'A' && NewHorizontal <= 'H' && NewVertical >= 1 && NewVertical <= 8)
					{
						FString NewHorizontalCoord;
						NewHorizontalCoord.AppendChar(NewHorizontal);

						if (MyBoardData.Contains(NewHorizontalCoord) && MyBoardData[NewHorizontalCoord].SecondMap.Contains(NewVertical))
						{
							ABoardSquare* TargetSquare = MyBoardData[NewHorizontalCoord].SecondMap[NewVertical];
							
							// Only allow moves that capture the pinning piece
							if (TargetSquare->GetOccupant() && TargetSquare->GetOccupant() == OwnerPiece->Attackers[0])
							{
								TargetSquare->ShowValidMoves(true);
								TargetSquare->HighlightSquare(true);
							}
						}
					}
				}
				return;
			}

			// Default unrestricted movement if not in check or pinned
			for (const FIntPoint& Move : KnightMoves)
			{
				TCHAR NewHorizontal = HorizontalChar + Move.X;
				int32 NewVertical = VerticalCoord + Move.Y;

				// Check if the new position is within bounds
				if (NewHorizontal >= 'A' && NewHorizontal <= 'H' && NewVertical >= 1 && NewVertical <= 8)
				{
					FString NewHorizontalCoord;
					NewHorizontalCoord.AppendChar(NewHorizontal);

					if (MyBoardData.Contains(NewHorizontalCoord) && MyBoardData[NewHorizontalCoord].SecondMap.Contains(NewVertical))
					{
						ABoardSquare* TargetSquare = MyBoardData[NewHorizontalCoord].SecondMap[NewVertical];
						
						// Allow movement if the square is empty or contains an opponent's piece
						if (!TargetSquare->GetOccupant())
						{
							TargetSquare->ShowValidMoves(true);
						}
						else if (TargetSquare->GetOccupant()->GetChessTeam() != OwnerPiece->GetChessTeam())
						{
							TargetSquare->ShowValidMoves(true);
							TargetSquare->HighlightSquare(true);
						}
					}
				}
			}
		}
	}
}

void UChessKnightBehaviour::HideMoves(ABoardSquare *CurrentBoardSquare)
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

			TArray<FIntPoint> KnightMoves = {
				{2, 1}, {2, -1}, {-2, 1}, {-2, -1},
				{1, 2}, {1, -2}, {-1, 2}, {-1, -2}
			};

			for (const FIntPoint& Move : KnightMoves)
			{
				int32 NewVertical = VerticalCoord + Move.Y;
				TCHAR NewHorizontal = HorizontalChar + Move.X;

				if (NewHorizontal >= 'A' && NewHorizontal <= 'H' && NewVertical >= 1 && NewVertical <= 8)
				{
					FString NewHorizontalCoord;
					NewHorizontalCoord.AppendChar(NewHorizontal);

					if (MyBoardData.Contains(NewHorizontalCoord) && MyBoardData[NewHorizontalCoord].SecondMap.Contains(NewVertical))
					{
						MyBoardData[NewHorizontalCoord].SecondMap[NewVertical]->ShowValidMoves(false);
					}
				}
			}
		}
	}
}

void UChessKnightBehaviour::MovePiece(ABoardSquare *DesiredMoveSquare)
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

void UChessKnightBehaviour::TakePiece(ABoardSquare *DesiredMoveSquare)
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

void UChessKnightBehaviour::CalculateNumberOfValidMoves(int &numberOfValidMoves)
{
    numberOfValidMoves = 0;  // Initialize valid move count

    ABaseChessPiece* OwnerPiece = Cast<ABaseChessPiece>(GetOwner());
    ABoardSquare* CurrentBoardSquare = OwnerPiece->GetCurrentBoardSquare();

    if (!CurrentBoardSquare) return;

    if (UWorld* World = GetWorld())
    {
        if (const AChessGameState* GameState = Cast<AChessGameState>(World->GetGameState<AGameStateBase>()))
        {
            // Get board data and current coordinates
            TMap<FString, FMyMapContainer>& MyBoardData = GameState->GetChessBoard()->GetChessBoardData();
            FString HorizontalCoord = CurrentBoardSquare->GetHorizontalCoordinate();
            int32 VerticalCoord = CurrentBoardSquare->GetVerticalCoordinate();
            TCHAR HorizontalChar = HorizontalCoord[0];

            ABaseChessPiece* King = (OwnerPiece->GetChessTeam() == EChessTeam::White) ? GameState->WhiteKing : GameState->BlackKing;
            bool bIsPinned = OwnerPiece->bIsPinned;

            // List of knight moves
            TArray<FIntPoint> KnightMoves = {
                {2, 1}, {2, -1}, {-2, 1}, {-2, -1},
                {1, 2}, {1, -2}, {-1, 2}, {-1, -2}
            };

            // 1. Case: King is in check and knight is not pinned
            if (King->bInCheck && !bIsPinned)
            {
                ABaseChessPiece* Attacker = King->Attackers[0];

                for (const FIntPoint& Move : KnightMoves)
                {
                    TCHAR NewHorizontal = HorizontalChar + Move.X;
                    int32 NewVertical = VerticalCoord + Move.Y;

                    // Ensure the move is within bounds
                    if (NewHorizontal >= 'A' && NewHorizontal <= 'H' && NewVertical >= 1 && NewVertical <= 8)
                    {
                        FString NewHorizontalCoord;
                        NewHorizontalCoord.AppendChar(NewHorizontal);

                        if (MyBoardData.Contains(NewHorizontalCoord) && MyBoardData[NewHorizontalCoord].SecondMap.Contains(NewVertical))
                        {
                            ABoardSquare* TargetSquare = MyBoardData[NewHorizontalCoord].SecondMap[NewVertical];

                            // If the knight can block the attack path or capture the attacker
                            if (TargetSquare == Attacker->GetCurrentBoardSquare())
                            {
                                numberOfValidMoves++;
                                if (numberOfValidMoves >= 1) return;  // Stop as soon as a valid move is found
                            }
                        }
                    }
                }
                return;  // Stop after calculating valid moves to block or capture the attacker
            }

            // 2. Case: King is in check and knight is pinned
            if (King->bInCheck && bIsPinned)
            {
                return;  // Knight cannot move when king is in check and it is pinned
            }

            // 3. Case: Knight is pinned but king is not in check
            if (bIsPinned)
            {
                return;  // Knights cannot break a pin, so no moves possible
            }

            // 4. Case: Unrestricted movement (not pinned or in check)
            for (const FIntPoint& Move : KnightMoves)
            {
                TCHAR NewHorizontal = HorizontalChar + Move.X;
                int32 NewVertical = VerticalCoord + Move.Y;

                // Ensure the move is within bounds
                if (NewHorizontal >= 'A' && NewHorizontal <= 'H' && NewVertical >= 1 && NewVertical <= 8)
                {
                    FString NewHorizontalCoord;
                    NewHorizontalCoord.AppendChar(NewHorizontal);

                    if (MyBoardData.Contains(NewHorizontalCoord) && MyBoardData[NewHorizontalCoord].SecondMap.Contains(NewVertical))
                    {
                        ABoardSquare* TargetSquare = MyBoardData[NewHorizontalCoord].SecondMap[NewVertical];

                        // If the square is empty or contains an opponent's piece
                        if (!TargetSquare->GetOccupant() || TargetSquare->GetOccupant()->GetChessTeam() != OwnerPiece->GetChessTeam())
                        {
                            numberOfValidMoves++;
                            if (numberOfValidMoves >= 1) return;  // Stop as soon as a valid move is found
                        }
                    }
                }
            }
        }
    }
}

void UChessKnightBehaviour::CanAttackSquare(ABoardSquare *TargetSquare, bool &bCanAttackSquare)
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
            int32 CurrentFile = CurrentSquare->GetVerticalCoordinate();
            TCHAR CurrentRank = CurrentSquare->GetHorizontalCoordinate()[0];

            int32 TargetFile = TargetSquare->GetVerticalCoordinate();
            TCHAR TargetRank = TargetSquare->GetHorizontalCoordinate()[0];

            // Knight moves in an "L" shape, so it can move either:
            // - 2 squares in one direction and 1 in the perpendicular direction
            int32 FileDifference = FMath::Abs(TargetFile - CurrentFile);
            int32 RankDifference = FMath::Abs(TargetRank - CurrentRank);

            // Valid knight moves must satisfy one of the two conditions:
            if ((FileDifference == 2 && RankDifference == 1) || (FileDifference == 1 && RankDifference == 2))
            {
                // Check if the target square is occupied by an opponent piece
                if (TargetSquare->GetOccupant() && TargetSquare->GetOccupant()->GetChessTeam() != Parent->GetChessTeam())
                {
                    bCanAttackSquare = true;
                    return;
                }

                // If the target square is unoccupied, the knight can still move there
                bCanAttackSquare = true;
                return;
            }
        }
    }

    bCanAttackSquare = false;
}

void UChessKnightBehaviour::CalculateIsPinned(ABoardSquare *KingBoardSquare)
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

void UChessKnightBehaviour::Server_MovePiece_Implementation(ABoardSquare *DesiredMoveSquare)
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

	if(AChessGameMode* GameMode = Cast<AChessGameMode>(GetWorld()->GetAuthGameMode()))
	{
		GameMode->EndTurn();
	}
}