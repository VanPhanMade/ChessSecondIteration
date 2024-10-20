// Copyright (©) 2024, Van Phan. All rights reserved.


#include "PlayerControllers/ChessPlayer.h"
#include "Widgets/InGameHUD.h"
#include "PlayerStates/ChessPlayerState.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameModes/ChessGameMode.h"
#include "Async/Async.h"

void AChessPlayer::BeginPlay() 
{
    Super::BeginPlay();

    if(IsLocalController() && InGameHUDWidget)
    {
        InGameHUDRef = CreateWidget<UInGameHUD>(this, InGameHUDWidget);
        if(InGameHUDRef) InGameHUDRef->AddToViewport();
    }

    if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(DefaultMappingContext, 0);
	}
}

void AChessPlayer::Server_PromotionChoice_Implementation(EPromotionType ChosePromotion)
{
    if(AChessGameMode* GameMode = Cast<AChessGameMode>(GetWorld()->GetAuthGameMode()))
    GameMode->PromotePawn(ChosePromotion);
}

void AChessPlayer::PromotionChoice(EPromotionType ChosePromotion)
{
    if(HasAuthority())
    {
        UE_LOG(LogTemp, Display, TEXT("Server promoting"));
        if(AChessGameMode* GameMode = Cast<AChessGameMode>(GetWorld()->GetAuthGameMode()))
        GameMode->PromotePawn(ChosePromotion);
    }
    else
    {
        UE_LOG(LogTemp, Display, TEXT("Client promoting"));
        Server_PromotionChoice(ChosePromotion);
    }
}

void AChessPlayer::UpdateUI()
{
    if(InGameHUDRef == nullptr) return;

    if(!bStartedGame) 
    {
        bStartedGame = true;
        AChessPlayerState* ChessPlayerState = GetPlayerState<AChessPlayerState>();
        if(ChessPlayerState)
        {
            InGameHUDRef->StartGame(ChessPlayerState->ChessTeam == EChessTeam::White);
        }
        FInputModeGameAndUI InputMode;
        InputMode.SetHideCursorDuringCapture(false);
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
        SetInputMode(InputMode);
        bShowMouseCursor = true;
    }
    else
    {
        InGameHUDRef->SwapTurns();
    }

    OnUpdateUI.Broadcast();
}

void AChessPlayer::UpdateUIEndGame(EEndGameState EndGameState)
{
    if(InGameHUDRef)
    {
        InGameHUDRef->GameOver(EndGameState);
    }
}

void AChessPlayer::Client_OpenPromotionUI_Implementation(ABaseChessPiece* PromotedPawn)
{
    if(InGameHUDRef)
    {
        InGameHUDRef->ShowPromotionPopup(PromotedPawn);
    }
}