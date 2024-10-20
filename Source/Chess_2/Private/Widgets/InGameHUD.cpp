// Copyright (©) 2024, Van Phan. All rights reserved.


#include "Widgets/InGameHUD.h"
#include "Components/TextBlock.h"
#include "PlayerStates/ChessPlayerState.h"
#include "GameStates/ChessGameState.h"
#include "Components/Overlay.h"
#include "PlayerControllers/ChessPlayer.h"
#include "Components/Button.h"

void UInGameHUD::StartGame(bool isWhite)
{
    if(StartGameWidgetAnim)
    {
        PlayerTurnState->SetText(isWhite? FText::FromName("Your turn") : FText::FromName("Waiting for opponent"));
        PlayAnimation(StartGameWidgetAnim);
    }
}

void UInGameHUD::SwapTurns()
{
    // Access the player state and game state
    APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
    AChessPlayerState* PlayerState = PlayerController ? PlayerController->GetPlayerState<AChessPlayerState>() : nullptr;
    AChessGameState* GameState = GetWorld()->GetGameState<AChessGameState>();

    if (PlayerState && GameState)
    {
        // Determine which king is in check based on the player's team
        bool bInCheck = (PlayerState->ChessTeam == EChessTeam::White) ? GameState->WhiteKing->bInCheck : GameState->BlackKing->bInCheck;

        // Update UI text based on check status and current turn
        FText NewText = FText::FromName(bInCheck ? "In Check!" : GameState->CurrentTeamTurn == PlayerState->ChessTeam ? "Your turn" : "Waiting for opponent");

        PlayerTurnState->SetText(NewText);
    }
}

void UInGameHUD::GameOver(EEndGameState PlayerEndState)
{
    if (AChessPlayer* Owner = Cast<AChessPlayer>(GetOwningPlayer())){
        FInputModeUIOnly InputMode;
        InputMode.SetWidgetToFocus(PawnPromotionOverlay->TakeWidget()); // Focus on the promotion overlay widget
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock); // Optional: Locks the mouse within the viewport

        Owner->SetInputMode(InputMode);  // Set the input mode to UI only
        Owner->bShowMouseCursor = true;  // Show the mouse cursor
    }
    
    switch(PlayerEndState)
    {
        case EEndGameState::Victory:
            PlayerTurnState->SetText(FText::FromName("Won by Checkmate!"));
            break;
        case EEndGameState::Defeat:
            PlayerTurnState->SetText(FText::FromName("Loss by Checkmate!"));
            break;
        case EEndGameState::Draw:
            PlayerTurnState->SetText(FText::FromName("Draw!"));
            break;
        default:
            break;
    }
}

void UInGameHUD::ShowPromotionPopup(ABaseChessPiece *PromotedPawn)
{
    PawnPromotionOverlay->SetVisibility(ESlateVisibility::Visible);
    if (AChessPlayer* Owner = Cast<AChessPlayer>(GetOwningPlayer()))
    {
        FInputModeUIOnly InputMode;
        InputMode.SetWidgetToFocus(PawnPromotionOverlay->TakeWidget()); // Focus on the promotion overlay widget
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock); // Optional: Locks the mouse within the viewport
    
        Owner->SetInputMode(InputMode);  // Set the input mode to UI only
        Owner->bShowMouseCursor = true;  // Show the mouse cursor
    }
}

bool UInGameHUD::Initialize()
{
    if(!Super::Initialize()) return false;

    // Ensure buttons are not null and properly set up
    if (BishopPromotion)
    {
        BishopPromotion->OnClicked.AddDynamic(this, &UInGameHUD::OnBishopButtonClicked);
    }

    if (QueenPromotion)
    {
        QueenPromotion->OnClicked.AddDynamic(this, &UInGameHUD::OnQueenButtonClicked);
    }

    if (RookPromotion)
    {
        RookPromotion->OnClicked.AddDynamic(this, &UInGameHUD::OnRookButtonClicked);
    }

    if (KnightPromotion)
    {
        KnightPromotion->OnClicked.AddDynamic(this, &UInGameHUD::OnKnightButtonClicked);
    }

    return true;
}

void UInGameHUD::OnQueenButtonClicked()
{
    if (AChessPlayer* Owner = Cast<AChessPlayer>(GetOwningPlayer()))
    {
        Owner->PromotionChoice(EPromotionType::Queen);
        PawnPromotionOverlay->SetVisibility(ESlateVisibility::Hidden);
        FInputModeGameOnly GameInputMode;
        Owner->SetInputMode(GameInputMode);
        Owner->bShowMouseCursor = true;
    }
}

void UInGameHUD::OnKnightButtonClicked()
{
    if (AChessPlayer* Owner = Cast<AChessPlayer>(GetOwningPlayer()))
    {
        Owner->PromotionChoice(EPromotionType::Knight);
        PawnPromotionOverlay->SetVisibility(ESlateVisibility::Hidden);
        FInputModeGameOnly GameInputMode;
        Owner->SetInputMode(GameInputMode);
        Owner->bShowMouseCursor = true;
    }
}

void UInGameHUD::OnRookButtonClicked()
{
    if (AChessPlayer* Owner = Cast<AChessPlayer>(GetOwningPlayer()))
    {
        Owner->PromotionChoice(EPromotionType::Rook);
        PawnPromotionOverlay->SetVisibility(ESlateVisibility::Hidden);
        FInputModeGameOnly GameInputMode;
        Owner->SetInputMode(GameInputMode);
        Owner->bShowMouseCursor = true;
    }
}

void UInGameHUD::OnBishopButtonClicked()
{
    if (AChessPlayer* Owner = Cast<AChessPlayer>(GetOwningPlayer()))
    {
        Owner->PromotionChoice(EPromotionType::Bishop);
        PawnPromotionOverlay->SetVisibility(ESlateVisibility::Hidden);
        FInputModeGameOnly GameInputMode;
        Owner->SetInputMode(GameInputMode);
        Owner->bShowMouseCursor = true;
    }
}
