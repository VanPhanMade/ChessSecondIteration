// Copyright (©) 2024, Van Phan. All rights reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */
class CHESS_2_API ChessHelperFunctions
{
public:
	static bool IsHorizontalLineBetween(class ABoardSquare* Start, class ABoardSquare* End, class UWorld* World);

    static bool IsVerticalLineBetween(class ABoardSquare* Start, class ABoardSquare* End, class UWorld* World);

    static bool IsDiagonalLineBetween(class ABoardSquare* Start, class ABoardSquare* End, class UWorld* World);
	
};
