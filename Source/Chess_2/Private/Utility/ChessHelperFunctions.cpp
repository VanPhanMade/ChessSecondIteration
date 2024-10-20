// Copyright (©) 2024, Van Phan. All rights reserved.


#include "Utility/ChessHelperFunctions.h"
#include "Actors/BoardSquare.h"
#include "GameStates/ChessGameState.h"
#include "Actors/ChessBoard.h"

// Doxygen comments

/// @brief Checks if theres a clear, uninterupted horizontal line between two squares
/// @param Start The starting square 
/// @param End The square we're checking towards
/// @return True if from Start to End there are no pieces in between
bool ChessHelperFunctions::IsHorizontalLineBetween(ABoardSquare *Start, ABoardSquare *End, UWorld* World)
{
    if (!World || Start->GetVerticalCoordinate() != End->GetVerticalCoordinate()) return false;
    if(Start == End) return false;

    if (const AChessGameState* GameState = Cast<AChessGameState>(World->GetGameState<AGameStateBase>()))
    {
        TMap<FString, FMyMapContainer>& MyBoardData = GameState->GetChessBoard()->GetChessBoardData();
        int32 VerticalCoord = Start->GetVerticalCoordinate();
        TCHAR StartHorizontal = Start->GetHorizontalCoordinate()[0];
        TCHAR EndHorizontal = End->GetHorizontalCoordinate()[0];

        int32 Direction = StartHorizontal < EndHorizontal ? 1 : -1;
        for (int32 Step = 1; Step < FMath::Abs(StartHorizontal - EndHorizontal); ++Step)
        {
            TCHAR IntermediateHorizontal = StartHorizontal + (Direction * Step);
            if (IntermediateHorizontal < 'A' || IntermediateHorizontal > 'H') return false;

            FString HorizontalCoord;
            HorizontalCoord.AppendChar(IntermediateHorizontal);

            if (MyBoardData.Contains(HorizontalCoord) && MyBoardData[HorizontalCoord].SecondMap.Contains(VerticalCoord))
            {
                ABoardSquare* IntermediateSquare = MyBoardData[HorizontalCoord].SecondMap[VerticalCoord];
                if (IntermediateSquare->GetOccupant()) return false;  // Occupant found
            }
        }
        return true;  // Clear path
    }
    return false;
}

/// @brief Checks if theres a clear, uninterupted vertical line between two squares
/// @param Start The starting square 
/// @param End The square we're checking towards
/// @return True if from Start to End there are no pieces in between
bool ChessHelperFunctions::IsVerticalLineBetween(ABoardSquare *Start, ABoardSquare *End, UWorld* World)
{
    if (!World || Start->GetHorizontalCoordinate() != End->GetHorizontalCoordinate()) return false;
    if(Start == End) return false;

    if (const AChessGameState* GameState = Cast<AChessGameState>(World->GetGameState<AGameStateBase>()))
    {
        TMap<FString, FMyMapContainer>& MyBoardData = GameState->GetChessBoard()->GetChessBoardData();
        int32 StartVertical = Start->GetVerticalCoordinate();
        int32 EndVertical = End->GetVerticalCoordinate();
        FString HorizontalCoord = Start->GetHorizontalCoordinate();

        int32 Direction = StartVertical < EndVertical ? 1 : -1;
        for (int32 Step = 1; Step < FMath::Abs(StartVertical - EndVertical); ++Step)
        {
            int32 IntermediateVertical = StartVertical + (Direction * Step);
            if (IntermediateVertical < 1 || IntermediateVertical > 8) return false;

            if (MyBoardData.Contains(HorizontalCoord) && MyBoardData[HorizontalCoord].SecondMap.Contains(IntermediateVertical))
            {
                ABoardSquare* IntermediateSquare = MyBoardData[HorizontalCoord].SecondMap[IntermediateVertical];
                if (IntermediateSquare->GetOccupant()) return false;  // Occupant found
            }
        }
        return true;  // Clear path
    }
    return false;
}

/// @brief Checks if theres a clear, uninterupted  diagonal line between two squares
/// @param Start The starting square 
/// @param End The square we're checking towards
/// @return True if from Start to End there are no pieces in between
bool ChessHelperFunctions::IsDiagonalLineBetween(ABoardSquare *Start, ABoardSquare *End, UWorld* World)
{
    if (!World) return false;
    if(Start == End) return false;

    int32 StartVertical = Start->GetVerticalCoordinate();
    int32 EndVertical = End->GetVerticalCoordinate();
    TCHAR StartHorizontal = Start->GetHorizontalCoordinate()[0];
    TCHAR EndHorizontal = End->GetHorizontalCoordinate()[0];

    if (FMath::Abs(StartVertical - EndVertical) != FMath::Abs(StartHorizontal - EndHorizontal)) return false;  // Ensure it's diagonal

    if (const AChessGameState* GameState = Cast<AChessGameState>(World->GetGameState<AGameStateBase>()))
    {
        TMap<FString, FMyMapContainer>& MyBoardData = GameState->GetChessBoard()->GetChessBoardData();

        int32 VerticalDirection = (StartVertical < EndVertical) ? 1 : -1;
        int32 HorizontalDirection = (StartHorizontal < EndHorizontal) ? 1 : -1;

        for (int32 Step = 1; Step < FMath::Abs(StartVertical - EndVertical); ++Step)
        {
            int32 IntermediateVertical = StartVertical + (VerticalDirection * Step);
            TCHAR IntermediateHorizontal = StartHorizontal + (HorizontalDirection * Step);
            if (IntermediateHorizontal < 'A' || IntermediateHorizontal > 'H' || IntermediateVertical < 1 || IntermediateVertical > 8) return false;

            FString HorizontalCoord;
            HorizontalCoord.AppendChar(IntermediateHorizontal);

            if (MyBoardData.Contains(HorizontalCoord) && MyBoardData[HorizontalCoord].SecondMap.Contains(IntermediateVertical))
            {
                ABoardSquare* IntermediateSquare = MyBoardData[HorizontalCoord].SecondMap[IntermediateVertical];
                if (IntermediateSquare->GetOccupant()) return false;  // Occupant found
            }
        }
        return true;  // Clear path
    }
    return false;
}