// Copyright (©) 2024, Van Phan. All rights reserved.


#include "ActorComponents/ChessPawnBehaviour.h"
#include "Actors/BaseChessPiece.h"
#include "GameStates/ChessGameState.h"
#include "Actors/ChessBoard.h"
#include "Actors/BoardSquare.h"
#include "GameModes/ChessGameMode.h"
#include "ActorComponents/ChessBishopBehaviour.h"
#include "ActorComponents/ChessQueenBehaviour.h"
#include "ActorComponents/ChessRookBehaviour.h"
#include "ActorComponents/ChessPawnBehaviour.h"
#include "ActorComponents/ChessKnightBehaviour.h"
#include "Utility/ChessHelperFunctions.h"

UChessPawnBehaviour::UChessPawnBehaviour()
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

void UChessPawnBehaviour::CalculateValidMoves(bool &bInCheck, bool &bIsPinned, bool &bIsProtected, bool &bHasMoved, ABoardSquare *CurrentBoardSquare)
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
            int32 ForwardDirection = (OwnerPiece->GetChessTeam() == EChessTeam::White) ? 1 : -1;
            ABaseChessPiece* King = (OwnerPiece->GetChessTeam() == EChessTeam::White) ? GameState->WhiteKing : GameState->BlackKing;
            // Single square forward movement
            int32 NewVertical = VerticalCoord + ForwardDirection;
            EPinDirection CurrentPinDirection = OwnerPiece->PinDirection;

            TArray<FIntPoint> DiagonalMoves = { {1, ForwardDirection}, {-1, ForwardDirection} };

            if(King->bInCheck) // Attempt to block/take attackers
            {   
                // no pieces are able to block and attack and escape an absolute pin
                if(OwnerPiece->PinDirection != EPinDirection::PinDirectionNone) return;  
                if(King->Attackers.Num() >= 2) // Only valid moves under a double attack is king moving
                {
                    return;
                }
                if (King->Attackers.Num() == 1)
                {
                    ABaseChessPiece* Attacker = King->Attackers[0];
                    // Handle different attack types
                    int HorizontalDifference = FMath::Abs(King->GetCurrentBoardSquare()->GetHorizontalCoordinate()[0] - Attacker->GetCurrentBoardSquare()->GetHorizontalCoordinate()[0]);
                    int VerticalDifference = FMath::Abs(King->GetCurrentBoardSquare()->GetVerticalCoordinate() - Attacker->GetCurrentBoardSquare()->GetVerticalCoordinate());
                    ABoardSquare* ForwardOne = MyBoardData.Contains(HorizontalCoord) && MyBoardData[HorizontalCoord].SecondMap.Contains(NewVertical) && !MyBoardData[HorizontalCoord].SecondMap[NewVertical]->GetOccupant() ? MyBoardData[HorizontalCoord].SecondMap[NewVertical] : nullptr;
                    
                    int32 TwoSquareForward = VerticalCoord + (2 * ForwardDirection);
                    ABoardSquare* ForwardTwo = !bHasMoved && MyBoardData.Contains(HorizontalCoord) && MyBoardData[HorizontalCoord].SecondMap.Contains(TwoSquareForward) && !MyBoardData[HorizontalCoord].SecondMap[TwoSquareForward]->GetOccupant() ? MyBoardData[HorizontalCoord].SecondMap[TwoSquareForward] : nullptr;
                    TArray<ABoardSquare*> ForwardSquares = { ForwardOne, ForwardTwo };

                    for (ABoardSquare* ForwardSquare : ForwardSquares)
                    {
                        if (!ForwardSquare) continue;  // Skip if null

                        if (HorizontalDifference == VerticalDifference && ChessHelperFunctions::IsDiagonalLineBetween(Attacker->GetCurrentBoardSquare(), ForwardSquare, World)
                        && ChessHelperFunctions::IsDiagonalLineBetween(King->GetCurrentBoardSquare(), ForwardSquare, World))
                        {
                            ForwardSquare->ShowValidMoves(true);
                            return;
                        }
                        else if (VerticalDifference == 0 && ChessHelperFunctions::IsHorizontalLineBetween(Attacker->GetCurrentBoardSquare(), ForwardSquare, World)
                        && ChessHelperFunctions::IsHorizontalLineBetween(King->GetCurrentBoardSquare(), ForwardSquare, World))
                        {
                            ForwardSquare->ShowValidMoves(true);
                            return;
                        }
                    }

                    for (const FIntPoint& Move : DiagonalMoves)
                    {
                        TCHAR NewHorizontal = HorizontalChar + Move.X;
                        NewVertical = VerticalCoord + Move.Y;

                        if (NewHorizontal < 'A' || NewHorizontal > 'H' || NewVertical < 1 || NewVertical > 8) continue;

                        FString NewHorizontalCoord;
                        NewHorizontalCoord.AppendChar(NewHorizontal);

                        if (MyBoardData.Contains(NewHorizontalCoord) && MyBoardData[NewHorizontalCoord].SecondMap.Contains(NewVertical))
                        {
                            ABoardSquare* TargetSquare = MyBoardData[NewHorizontalCoord].SecondMap[NewVertical];
                            if (TargetSquare == Attacker->GetCurrentBoardSquare())
                            {
                                TargetSquare->ShowValidMoves(true);
                                TargetSquare->HighlightSquare(true);
                            }
                        }
                    }
                    if(Attacker->FindComponentByClass<UChessPawnBehaviour>())
                    {   
                        // Reserved for check if an en passant can be done to take that attacking piece
                        return;
                    }
                }
            }
            else // King no in check
            {
                if(bIsPinned) 
                {
                    if(CurrentPinDirection == EPinDirection::Vertical) // can still move forward but cannot move diagonally for attacks
                    {
                        // Check if the new position is within bounds
                        if (NewVertical >= 1 && NewVertical <= 8)
                        {
                            // Forward move (no capturing)
                            if (MyBoardData.Contains(HorizontalCoord) && MyBoardData[HorizontalCoord].SecondMap.Contains(NewVertical))
                            {
                                ABoardSquare* TargetSquare = MyBoardData[HorizontalCoord].SecondMap[NewVertical];
                                if (!TargetSquare->GetOccupant())
                                {
                                    TargetSquare->ShowValidMoves(true);
                                }
                            }
                        }

                        // Two square move from starting position
                        if (!bHasMoved)  // Only move two squares if the pawn hasn't moved yet
                        {
                            int32 TwoSquareForward = VerticalCoord + (2 * ForwardDirection);
                            if (TwoSquareForward >= 1 && TwoSquareForward <= 8)
                            {
                                if (MyBoardData.Contains(HorizontalCoord) && MyBoardData[HorizontalCoord].SecondMap.Contains(TwoSquareForward))
                                {
                                    ABoardSquare* TargetSquare = MyBoardData[HorizontalCoord].SecondMap[TwoSquareForward];
                                    if (!TargetSquare->GetOccupant())
                                    {
                                        TargetSquare->ShowValidMoves(true);
                                    }
                                }
                            }
                        }
                    }
                    if(CurrentPinDirection == EPinDirection::Horizontal) // since pawn can never exit pins moving horizontally
                    {
                        return; 
                    }
                    if(CurrentPinDirection == EPinDirection::Diagonal) // Check if we can take the piece in the diagonal pin, if not then we cannot move
                    {
                        for (const FIntPoint& Move : DiagonalMoves)
                        {
                            TCHAR NewHorizontal = HorizontalChar + Move.X;
                            NewVertical = VerticalCoord + Move.Y;

                            if (NewHorizontal >= 'A' && NewHorizontal <= 'H' && NewVertical >= 1 && NewVertical <= 8)
                            {
                                FString NewHorizontalCoord;
                                NewHorizontalCoord.AppendChar(NewHorizontal);

                                if (MyBoardData.Contains(NewHorizontalCoord) && MyBoardData[NewHorizontalCoord].SecondMap.Contains(NewVertical))
                                {
                                    ABoardSquare* TargetSquare = MyBoardData[NewHorizontalCoord].SecondMap[NewVertical];
                                    // Check if the square contains an opponent's piece
                                    if (TargetSquare->GetOccupant() 
                                    && TargetSquare->GetOccupant()->GetChessTeam() != OwnerPiece->GetChessTeam() 
                                    && TargetSquare == OwnerPiece->Attackers[0]->GetCurrentBoardSquare())
                                    {
                                        TargetSquare->ShowValidMoves(true);
                                        TargetSquare->HighlightSquare(true);
                                    }
                                }
                            }
                        }
                        return;
                    }
                }
                else // Default unrestricted movement
                {
                    // Check if the new position is within bounds
                    if (NewVertical >= 1 && NewVertical <= 8)
                    {
                        // Forward move (no capturing)
                        if (MyBoardData.Contains(HorizontalCoord) && MyBoardData[HorizontalCoord].SecondMap.Contains(NewVertical))
                        {
                            ABoardSquare* TargetSquare = MyBoardData[HorizontalCoord].SecondMap[NewVertical];
                            if (!TargetSquare->GetOccupant())
                            {
                                TargetSquare->ShowValidMoves(true);
                            }
                        }
                    }

                    // Two square move from starting position
                    if (!bHasMoved)  // Only move two squares if the pawn hasn't moved yet
                    {
                        int32 TwoSquareForward = VerticalCoord + (2 * ForwardDirection);
                        if (TwoSquareForward >= 1 && TwoSquareForward <= 8)
                        {
                            if (MyBoardData.Contains(HorizontalCoord) && MyBoardData[HorizontalCoord].SecondMap.Contains(TwoSquareForward))
                            {
                                ABoardSquare* TargetSquare = MyBoardData[HorizontalCoord].SecondMap[TwoSquareForward];
                                if (!TargetSquare->GetOccupant())
                                {
                                    TargetSquare->ShowValidMoves(true);
                                }
                            }
                        }
                    }

                    for (const FIntPoint& Move : DiagonalMoves)
                    {
                        TCHAR NewHorizontal = HorizontalChar + Move.X;
                        NewVertical = VerticalCoord + Move.Y;

                        if (NewHorizontal >= 'A' && NewHorizontal <= 'H' && NewVertical >= 1 && NewVertical <= 8)
                        {
                            FString NewHorizontalCoord;
                            NewHorizontalCoord.AppendChar(NewHorizontal);

                            if (MyBoardData.Contains(NewHorizontalCoord) && MyBoardData[NewHorizontalCoord].SecondMap.Contains(NewVertical))
                            {
                                ABoardSquare* TargetSquare = MyBoardData[NewHorizontalCoord].SecondMap[NewVertical];
                                // Check if the square contains an opponent's piece
                                if (TargetSquare->GetOccupant() && TargetSquare->GetOccupant()->GetChessTeam() != OwnerPiece->GetChessTeam())
                                {
                                    TargetSquare->ShowValidMoves(true);
                                    TargetSquare->HighlightSquare(true);
                                }
                            }
                        }
                    }

                    // Calculate if en passant is valid
                    if (GameState->LastTwoMovePawn != nullptr)
                    {
                        // Check if the last moved pawn belongs to the opposing team
                        ABaseChessPiece* LastPawn = GameState->LastTwoMovePawn;
                        if (LastPawn->GetChessTeam() != OwnerPiece->GetChessTeam()) // Ensure the last moved pawn is an opponent's pawn
                        {
                            FString LastPawnHorizontalCoord = LastPawn->GetCurrentBoardSquare()->GetHorizontalCoordinate();
                            int32 LastPawnVerticalCoord = LastPawn->GetCurrentBoardSquare()->GetVerticalCoordinate();

                            // Check if the last moved pawn is adjacent horizontally but on the same vertical row
                            if (VerticalCoord == LastPawnVerticalCoord && FMath::Abs(HorizontalChar - LastPawnHorizontalCoord[0]) == 1)
                            {
                                // The opponent's pawn is directly adjacent (left or right). Now check if en passant is possible.

                                // Calculate the target square diagonally behind the opponent's pawn (for en passant capture)
                                int32 EnPassantVertical = VerticalCoord + ForwardDirection; // ForwardDirection is +1 for White, -1 for Black
                                FString EnPassantHorizontalCoord;
                                EnPassantHorizontalCoord.AppendChar(LastPawnHorizontalCoord[0]); // Same horizontal as the last moved pawn

                                // Ensure the target square (diagonally behind) is within bounds
                                if (EnPassantVertical >= 1 && EnPassantVertical <= 8)
                                {
                                    // Check if the target square is empty for en passant capture
                                    if (MyBoardData.Contains(EnPassantHorizontalCoord) && MyBoardData[EnPassantHorizontalCoord].SecondMap.Contains(EnPassantVertical))
                                    {
                                        ABoardSquare* EnPassantTargetSquare = MyBoardData[EnPassantHorizontalCoord].SecondMap[EnPassantVertical];

                                        // Ensure the target square is empty (for en passant)
                                        if (!EnPassantTargetSquare->GetOccupant())
                                        {
                                            // Show valid en passant move
                                            EnPassantTargetSquare->ShowValidMoves(true);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

void UChessPawnBehaviour::HideMoves(ABoardSquare *CurrentBoardSquare)
{
	if (!CurrentBoardSquare) return;

    if (const UWorld* World = GetWorld())
    {
        if (const AChessGameState* GameState = Cast<AChessGameState>(World->GetGameState<AGameStateBase>()))
        {
            TMap<FString, FMyMapContainer>& MyBoardData = GameState->GetChessBoard()->GetChessBoardData();
            FString HorizontalCoord = CurrentBoardSquare->GetHorizontalCoordinate();
            int32 VerticalCoord = CurrentBoardSquare->GetVerticalCoordinate();
            TCHAR HorizontalChar = HorizontalCoord[0];
			MyBoardData[HorizontalCoord].SecondMap[VerticalCoord]->HighlightSquare(false); 

            ABaseChessPiece* OwnerPiece = Cast<ABaseChessPiece>(GetOwner());
            int32 ForwardDirection = (OwnerPiece->GetChessTeam() == EChessTeam::White) ? 1 : -1;

            // Single square forward movement
            int32 NewVertical = VerticalCoord + ForwardDirection;

            if (NewVertical >= 1 && NewVertical <= 8)
            {
                // Forward move (no capturing)
                if (MyBoardData.Contains(HorizontalCoord) && MyBoardData[HorizontalCoord].SecondMap.Contains(NewVertical))
                {
                    ABoardSquare* TargetSquare = MyBoardData[HorizontalCoord].SecondMap[NewVertical];
					TargetSquare->ShowValidMoves(false);
                }
            }

			// Two square forward movement
			int32 TwoSquareForward = VerticalCoord + (2 * ForwardDirection);
			if (TwoSquareForward >= 1 && TwoSquareForward <= 8)
			{
				if (MyBoardData.Contains(HorizontalCoord) && MyBoardData[HorizontalCoord].SecondMap.Contains(TwoSquareForward))
				{
					ABoardSquare* TargetSquare = MyBoardData[HorizontalCoord].SecondMap[TwoSquareForward];
					TargetSquare->ShowValidMoves(false);
				}
			}

            // Diagonal capturing moves
            TArray<FIntPoint> DiagonalMoves = {
                {1, ForwardDirection}, {-1, ForwardDirection}
            };

            for (const FIntPoint& Move : DiagonalMoves)
            {
                TCHAR NewHorizontal = HorizontalChar + Move.X;
                NewVertical = VerticalCoord + Move.Y;

                if (NewHorizontal >= 'A' && NewHorizontal <= 'H' && NewVertical >= 1 && NewVertical <= 8)
                {
                    FString NewHorizontalCoord;
                    NewHorizontalCoord.AppendChar(NewHorizontal);

                    if (MyBoardData.Contains(NewHorizontalCoord) && MyBoardData[NewHorizontalCoord].SecondMap.Contains(NewVertical))
                    {
                        ABoardSquare* TargetSquare = MyBoardData[NewHorizontalCoord].SecondMap[NewVertical];
						TargetSquare->ShowValidMoves(false);
                    }
                }
            }
        }
    }
}

void UChessPawnBehaviour::MovePiece(ABoardSquare *DesiredMoveSquare)
{
	// Desired squares can only be valid moves since they're calculated on show moves
	if(!DesiredMoveSquare) return;
    UWorld* World = GetWorld();
    AChessGameState* GameState = Cast<AChessGameState>(World->GetGameState<AGameStateBase>());
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
        if(FMath::Abs(Parent->GetCurrentBoardSquare()->GetVerticalCoordinate() - DesiredMoveSquare->GetVerticalCoordinate()) == 2
        && Parent->GetCurrentBoardSquare()->GetHorizontalCoordinate() == DesiredMoveSquare->GetHorizontalCoordinate())
        {
            if (World)
            if (GameState)
            {
                GameState->LastTwoMovePawn = Parent;
            }
            else{
                GameState->LastTwoMovePawn = nullptr;
            }
        }
        else
        {
            if (World)
            {
                if (GameState)
                {
                    // Check if this is an en passant move
                    ABaseChessPiece* LastPawn = GameState->LastTwoMovePawn;
                    if (LastPawn && LastPawn->GetChessTeam() != Parent->GetChessTeam())
                    {
                        // Check if the move is a diagonal move to an empty square
                        int32 VerticalDiff = FMath::Abs(Parent->GetCurrentBoardSquare()->GetVerticalCoordinate() - DesiredMoveSquare->GetVerticalCoordinate());
                        int32 HorizontalDiff = FMath::Abs(Parent->GetCurrentBoardSquare()->GetHorizontalCoordinate()[0] - DesiredMoveSquare->GetHorizontalCoordinate()[0]);

                        // ensure the move is an empty diagonal
                        if (VerticalDiff == 1 && HorizontalDiff == 1 && !DesiredMoveSquare->GetOccupant())
                        {
                            // The move is diagonal and the target square is empty, now confirm it's en passant
                            TCHAR LastPawnHorizontalChar = LastPawn->GetCurrentBoardSquare()->GetHorizontalCoordinate()[0];
                            TCHAR ParentHorizontalChar = Parent->GetCurrentBoardSquare()->GetHorizontalCoordinate()[0];

                            if ((LastPawnHorizontalChar == ParentHorizontalChar + 1 || LastPawnHorizontalChar == ParentHorizontalChar - 1) &&
                                LastPawn->GetCurrentBoardSquare()->GetVerticalCoordinate() == Parent->GetCurrentBoardSquare()->GetVerticalCoordinate())
                            {
                                // En passant confirmed! Capture the last moved pawn
                                LastPawn->GetCurrentBoardSquare()->Occupant = nullptr;
                                LastPawn->TakeChessPiece();
                                GameState->LastTwoMovePawn = nullptr;

                                UE_LOG(LogTemp, Display, TEXT("En passant capture performed!"));
                            }
                            else
                            {
                                // Debugging outputs for comparison of coordinates
                                UE_LOG(LogTemp, Display, TEXT("En passant piece not on correct spot?"));

                                UE_LOG(LogTemp, Display, TEXT("LastPawn's Horizontal: %s, Parent's Horizontal: %s"), 
                                    *LastPawn->GetCurrentBoardSquare()->GetHorizontalCoordinate(), 
                                    *Parent->GetCurrentBoardSquare()->GetHorizontalCoordinate());
                                
                                UE_LOG(LogTemp, Display, TEXT("LastPawn's Vertical: %d, Parent's Vertical: %d"), 
                                    LastPawn->GetCurrentBoardSquare()->GetVerticalCoordinate(), 
                                    Parent->GetCurrentBoardSquare()->GetVerticalCoordinate());
                            }
                        }
                    }
                }
            }
        }

        // If the destination square has an occupant (capture logic)
		if(DesiredMoveSquare->GetOccupant()) DesiredMoveSquare->GetOccupant()->TakeChessPiece();

        // Move the pawn to the new square
		Parent->GetCurrentBoardSquare()->Occupant = nullptr;
		DesiredMoveSquare->Occupant = Parent;
		Parent->CurrentBoardSquare = DesiredMoveSquare;
        Parent->bHasMoved = true;

		if(AChessGameMode* GameMode = Cast<AChessGameMode>(GetWorld()->GetAuthGameMode()))
        {
            // Indicates pawn moved to promoting square
            if(DesiredMoveSquare->GetVerticalCoordinate() == 1 || DesiredMoveSquare->GetVerticalCoordinate() == 8)
            {
                if (GameState)
                {
                    GameState->PromotePawn(Parent);
                }
            }
            else
            {
                GameMode->EndTurn();
            }
            
        }
         
	}
	else{
		Server_MovePiece(DesiredMoveSquare);
	}
}

void UChessPawnBehaviour::TakePiece(ABoardSquare *CurrentBoardSquare)
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

void UChessPawnBehaviour::CanAttackSquare(ABoardSquare* TargetSquare, bool& bCanAttackSquare)
{
    ABaseChessPiece* Parent = Cast<ABaseChessPiece>(GetOwner());
    if (!Parent || !TargetSquare) return;

    // Get the current square (starting point of the pawn)
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
            // Access the ChessBoardData TMap
            TMap<FString, FMyMapContainer>& MyBoardData = GameState->GetChessBoard()->GetChessBoardData();

            // Get the current square's coordinates
            FString HorizontalCoord = CurrentSquare->GetHorizontalCoordinate();  // 'A', 'B', etc.
            int32 VerticalCoord = CurrentSquare->GetVerticalCoordinate();        // 1, 2, etc.

            TCHAR CurrentFile = HorizontalCoord[0];  // 'A', 'B', etc.
            int32 CurrentRank = VerticalCoord;

            // Get target square coordinates
            FString TargetHorizontalCoord = TargetSquare->GetHorizontalCoordinate();
            int32 TargetVerticalCoord = TargetSquare->GetVerticalCoordinate();

            TCHAR TargetFile = TargetHorizontalCoord[0];
            int32 TargetRank = TargetVerticalCoord;

            // White Pawn attacks diagonally forward
            if (Parent->GetChessTeam() == EChessTeam::White)
            {
                // Check if the target is diagonally in front (one rank ahead, one file left or right)
                if ((TargetRank == CurrentRank + 1) &&
                    (TargetFile == CurrentFile + 1 || TargetFile == CurrentFile - 1))
                {
                    bCanAttackSquare = true;
                    return;
                }
            }
            // Black Pawn attacks diagonally forward (but moves in the opposite direction)
            else if (Parent->GetChessTeam() == EChessTeam::Black)
            {
                // Check if the target is diagonally in front (one rank behind, one file left or right)
                if ((TargetRank == CurrentRank - 1) &&
                    (TargetFile == CurrentFile + 1 || TargetFile == CurrentFile - 1))
                {
                    bCanAttackSquare = true;
                    return;
                }
            }
        }
    }

    // If none of the conditions match, the pawn cannot attack the target square
    bCanAttackSquare = false;
}

void UChessPawnBehaviour::CalculateIsPinned(ABoardSquare *KingBoardSquare)
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

void UChessPawnBehaviour::CalculateNumberOfValidMoves(int &numberOfValidMoves)
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

            int32 ForwardDirection = (OwnerPiece->GetChessTeam() == EChessTeam::White) ? 1 : -1;
            ABaseChessPiece* King = (OwnerPiece->GetChessTeam() == EChessTeam::White) ? GameState->WhiteKing : GameState->BlackKing;
            int32 NewVertical = VerticalCoord + ForwardDirection;
            EPinDirection CurrentPinDirection = OwnerPiece->PinDirection;
            TArray<FIntPoint> DiagonalMoves = { {1, ForwardDirection}, {-1, ForwardDirection} };

            // 1. Case: King is in check
            if (King->bInCheck) 
            {
                if (OwnerPiece->PinDirection != EPinDirection::PinDirectionNone) return;  // Can't move if pinned
                if (King->Attackers.Num() >= 2) return; // Only valid moves under double attack is king moving

                if (King->Attackers.Num() == 1)
                {
                    ABaseChessPiece* Attacker = King->Attackers[0];

                    // Check forward move
                    ABoardSquare* ForwardOne = MyBoardData.Contains(HorizontalCoord) && MyBoardData[HorizontalCoord].SecondMap.Contains(NewVertical) 
                        && !MyBoardData[HorizontalCoord].SecondMap[NewVertical]->GetOccupant() 
                        ? MyBoardData[HorizontalCoord].SecondMap[NewVertical] 
                        : nullptr;

                    if (ForwardOne) 
                    {
                        int HorizontalDifference = FMath::Abs(King->GetCurrentBoardSquare()->GetHorizontalCoordinate()[0] - Attacker->GetCurrentBoardSquare()->GetHorizontalCoordinate()[0]);
                        int VerticalDifference = FMath::Abs(King->GetCurrentBoardSquare()->GetVerticalCoordinate() - Attacker->GetCurrentBoardSquare()->GetVerticalCoordinate());

                        if (HorizontalDifference == VerticalDifference && ChessHelperFunctions::IsDiagonalLineBetween(Attacker->GetCurrentBoardSquare(), ForwardOne, World)
                            && ChessHelperFunctions::IsDiagonalLineBetween(King->GetCurrentBoardSquare(), ForwardOne, World))
                        {
                            numberOfValidMoves++;
                            if (numberOfValidMoves >= 1) return; // Stop as soon as a valid move is found
                        }
                        else if (VerticalDifference == 0 && ChessHelperFunctions::IsHorizontalLineBetween(Attacker->GetCurrentBoardSquare(), ForwardOne, World)
                            && ChessHelperFunctions::IsHorizontalLineBetween(King->GetCurrentBoardSquare(), ForwardOne, World))
                        {
                            numberOfValidMoves++;
                            if (numberOfValidMoves >= 1) return; // Stop as soon as a valid move is found
                        }
                    }

                    // Check diagonal captures
                    for (const FIntPoint& Move : DiagonalMoves)
                    {
                        TCHAR NewHorizontal = HorizontalChar + Move.X;
                        NewVertical = VerticalCoord + Move.Y;

                        if (NewHorizontal >= 'A' && NewHorizontal <= 'H' && NewVertical >= 1 && NewVertical <= 8)
                        {
                            FString NewHorizontalCoord;
                            NewHorizontalCoord.AppendChar(NewHorizontal);

                            if (MyBoardData.Contains(NewHorizontalCoord) && MyBoardData[NewHorizontalCoord].SecondMap.Contains(NewVertical))
                            {
                                ABoardSquare* TargetSquare = MyBoardData[NewHorizontalCoord].SecondMap[NewVertical];
                                if (TargetSquare == Attacker->GetCurrentBoardSquare())
                                {
                                    numberOfValidMoves++;
                                    if (numberOfValidMoves >= 1) return; // Stop as soon as a valid move is found
                                }
                            }
                        }
                    }
                }
                return; // Stop after calculating valid moves to block or capture the attacker
            }

            // 2. Case: Not in check, but pinned
            if (OwnerPiece->bIsPinned)
            {
                if (CurrentPinDirection == EPinDirection::Vertical) 
                {
                    // Forward move (no capturing)
                    if (NewVertical >= 1 && NewVertical <= 8)
                    {
                        if (MyBoardData.Contains(HorizontalCoord) && MyBoardData[HorizontalCoord].SecondMap.Contains(NewVertical))
                        {
                            ABoardSquare* TargetSquare = MyBoardData[HorizontalCoord].SecondMap[NewVertical];
                            if (!TargetSquare->GetOccupant())
                            {
                                numberOfValidMoves++;
                                if (numberOfValidMoves >= 1) return; // Stop as soon as a valid move is found
                            }
                        }
                    }
                }
                else if (CurrentPinDirection == EPinDirection::Diagonal) 
                {
                    for (const FIntPoint& Move : DiagonalMoves)
                    {
                        TCHAR NewHorizontal = HorizontalChar + Move.X;
                        NewVertical = VerticalCoord + Move.Y;

                        if (NewHorizontal >= 'A' && NewHorizontal <= 'H' && NewVertical >= 1 && NewVertical <= 8)
                        {
                            FString NewHorizontalCoord;
                            NewHorizontalCoord.AppendChar(NewHorizontal);

                            if (MyBoardData.Contains(NewHorizontalCoord) && MyBoardData[NewHorizontalCoord].SecondMap.Contains(NewVertical))
                            {
                                ABoardSquare* TargetSquare = MyBoardData[NewHorizontalCoord].SecondMap[NewVertical];
                                // Check if the square contains an opponent's piece
                                if (TargetSquare->GetOccupant() && TargetSquare->GetOccupant()->GetChessTeam() != OwnerPiece->GetChessTeam())
                                {
                                    numberOfValidMoves++;
                                    if (numberOfValidMoves >= 1) return; // Stop as soon as a valid move is found
                                }
                            }
                        }
                    }
                }
                return; // Can't move further if pinned
            }

            // 3. Case: Unrestricted movement
            // Check forward move
            if (NewVertical >= 1 && NewVertical <= 8)
            {
                if (MyBoardData.Contains(HorizontalCoord) && MyBoardData[HorizontalCoord].SecondMap.Contains(NewVertical))
                {
                    ABoardSquare* TargetSquare = MyBoardData[HorizontalCoord].SecondMap[NewVertical];
                    if (!TargetSquare->GetOccupant())
                    {
                        numberOfValidMoves++;
                        if (numberOfValidMoves >= 1) return; // Stop as soon as a valid move is found
                    }
                }
            }

            // Check two square move from starting position
            if (!OwnerPiece->bHasMoved)
            {
                int32 TwoSquareForward = VerticalCoord + (2 * ForwardDirection);
                if (TwoSquareForward >= 1 && TwoSquareForward <= 8)
                {
                    if (MyBoardData.Contains(HorizontalCoord) && MyBoardData[HorizontalCoord].SecondMap.Contains(TwoSquareForward))
                    {
                        ABoardSquare* TargetSquare = MyBoardData[HorizontalCoord].SecondMap[TwoSquareForward];
                        if (!TargetSquare->GetOccupant())
                        {
                            numberOfValidMoves++;
                            if (numberOfValidMoves >= 1) return; // Stop as soon as a valid move is found
                        }
                    }
                }
            }

            // Check diagonal captures
            for (const FIntPoint& Move : DiagonalMoves)
            {
                TCHAR NewHorizontal = HorizontalChar + Move.X;
                NewVertical = VerticalCoord + Move.Y;

                if (NewHorizontal >= 'A' && NewHorizontal <= 'H' && NewVertical >= 1 && NewVertical <= 8)
                {
                    FString NewHorizontalCoord;
                    NewHorizontalCoord.AppendChar(NewHorizontal);

                    if (MyBoardData.Contains(NewHorizontalCoord) && MyBoardData[NewHorizontalCoord].SecondMap.Contains(NewVertical))
                    {
                        ABoardSquare* TargetSquare = MyBoardData[NewHorizontalCoord].SecondMap[NewVertical];
                        if (TargetSquare->GetOccupant() && TargetSquare->GetOccupant()->GetChessTeam() != OwnerPiece->GetChessTeam())
                        {
                            numberOfValidMoves++;
                            if (numberOfValidMoves >= 1) return; // Stop as soon as a valid move is found
                        }
                    }
                }
            }

            // Check for en passant
            if (GameState->LastTwoMovePawn != nullptr)
            {
                ABaseChessPiece* LastPawn = GameState->LastTwoMovePawn;
                if (LastPawn->GetChessTeam() != OwnerPiece->GetChessTeam()) // Ensure the last moved pawn is an opponent's pawn
                {
                    FString LastPawnHorizontalCoord = LastPawn->GetCurrentBoardSquare()->GetHorizontalCoordinate();
                    int32 LastPawnVerticalCoord = LastPawn->GetCurrentBoardSquare()->GetVerticalCoordinate();

                    // Check if the last moved pawn is adjacent horizontally but on the same vertical row
                    if (VerticalCoord == LastPawnVerticalCoord && FMath::Abs(HorizontalChar - LastPawnHorizontalCoord[0]) == 1)
                    {
                        numberOfValidMoves++;
                        if (numberOfValidMoves >= 1) return; // Stop as soon as a valid move is found
                    }
                }
            }
        }
    }
}

void UChessPawnBehaviour::Server_MovePiece_Implementation(ABoardSquare *DesiredMoveSquare)
{
	if(!DesiredMoveSquare) return;
    UWorld* World = GetWorld();
    AChessGameState* GameState = Cast<AChessGameState>(World->GetGameState<AGameStateBase>());
	ABaseChessPiece* Parent = Cast<ABaseChessPiece>(GetOwner());
	if(Parent)
	{
		FVector NewReleasePoint = DesiredMoveSquare->GetActorLocation();
		NewReleasePoint.Z = Parent->GetActorLocation().Z;
		Parent->SetActorLocation(NewReleasePoint);
	}
    if(FMath::Abs(Parent->GetCurrentBoardSquare()->GetVerticalCoordinate() - DesiredMoveSquare->GetVerticalCoordinate()) == 2
    && Parent->GetCurrentBoardSquare()->GetHorizontalCoordinate() == DesiredMoveSquare->GetHorizontalCoordinate())
    {
        if (World && GameState)
        {
            GameState->LastTwoMovePawn = Parent;
        }
    }
    else
        {
            if (World && GameState)
            {
                // Check if this is an en passant move
                ABaseChessPiece* LastPawn = GameState->LastTwoMovePawn;
                if (LastPawn && LastPawn->GetChessTeam() != Parent->GetChessTeam())
                {
                    // Check if the move is a diagonal move to an empty square
                    int32 VerticalDiff = FMath::Abs(Parent->GetCurrentBoardSquare()->GetVerticalCoordinate() - DesiredMoveSquare->GetVerticalCoordinate());
                    int32 HorizontalDiff = FMath::Abs(Parent->GetCurrentBoardSquare()->GetHorizontalCoordinate()[0] - DesiredMoveSquare->GetHorizontalCoordinate()[0]);

                    // ensure the move is diagonal
                    if (VerticalDiff == 1 && HorizontalDiff == 1 && !DesiredMoveSquare->GetOccupant())
                    {
                        // The move is diagonal and the target square is empty, now confirm it's en passant
                        TCHAR LastPawnHorizontalChar = LastPawn->GetCurrentBoardSquare()->GetHorizontalCoordinate()[0];
                        TCHAR ParentHorizontalChar = Parent->GetCurrentBoardSquare()->GetHorizontalCoordinate()[0];

                        if ((LastPawnHorizontalChar == ParentHorizontalChar + 1 || LastPawnHorizontalChar == ParentHorizontalChar - 1) &&
                            LastPawn->GetCurrentBoardSquare()->GetVerticalCoordinate() == Parent->GetCurrentBoardSquare()->GetVerticalCoordinate())
                        {
                            // En passant confirmed! Capture the last moved pawn
                            LastPawn->GetCurrentBoardSquare()->Occupant = nullptr;
                            LastPawn->TakeChessPiece();
                            GameState->LastTwoMovePawn = nullptr;
                        }
                        else
                        {
                            // Debugging outputs for comparison of coordinates
                            UE_LOG(LogTemp, Display, TEXT("En passant piece not on correct spot?"));

                            UE_LOG(LogTemp, Display, TEXT("LastPawn's Horizontal: %s, Parent's Horizontal: %s"), 
                                *LastPawn->GetCurrentBoardSquare()->GetHorizontalCoordinate(), 
                                *Parent->GetCurrentBoardSquare()->GetHorizontalCoordinate());
                            
                            UE_LOG(LogTemp, Display, TEXT("LastPawn's Vertical: %d, Parent's Vertical: %d"), 
                                LastPawn->GetCurrentBoardSquare()->GetVerticalCoordinate(), 
                                Parent->GetCurrentBoardSquare()->GetVerticalCoordinate());
                        }
                    }
                }
            }
        }

    // capture
    if(DesiredMoveSquare->GetOccupant()) DesiredMoveSquare->GetOccupant()->TakeChessPiece();

    // Move pawn
    Parent->GetCurrentBoardSquare()->Occupant = nullptr;
    DesiredMoveSquare->Occupant = Parent;
    Parent->CurrentBoardSquare = DesiredMoveSquare;
    Parent->bHasMoved = true;

    if(AChessGameMode* GameMode = Cast<AChessGameMode>(GetWorld()->GetAuthGameMode()))
    {
        if (GameState)
        {
            GameState->PromotePawn(Parent);
        }
        else
        {
            GameMode->EndTurn();
        }
        
    } 
    
}
