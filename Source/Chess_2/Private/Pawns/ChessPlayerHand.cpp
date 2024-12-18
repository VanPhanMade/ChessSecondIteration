// Copyright (©) 2024, Van Phan. All rights reserved.


#include "Pawns/ChessPlayerHand.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

AChessPlayerHand::AChessPlayerHand()
{
	PrimaryActorTick.bCanEverTick = true;

}

void AChessPlayerHand::BeginPlay()
{
	Super::BeginPlay();
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
	
}

void AChessPlayerHand::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AChessPlayerHand::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(MovementAction, ETriggerEvent::Triggered, this, &ThisClass::MoveCallback);
		EnhancedInputComponent->BindAction(MouseAction, ETriggerEvent::Triggered, this, &ThisClass::MouseCallback);
		EnhancedInputComponent->BindAction(LeftClickAction, ETriggerEvent::Triggered, this, &ThisClass::LeftClickCallback);
		EnhancedInputComponent->BindAction(RightClickAction, ETriggerEvent::Triggered, this, &ThisClass::RightClickCallback);
	}
}

void AChessPlayerHand::MoveCallback(const FInputActionValue &Value)
{
	UE_LOG(LogTemp, Display, TEXT("Moving"));

}

void AChessPlayerHand::MouseCallback(const FInputActionValue &Value)
{
	UE_LOG(LogTemp, Display, TEXT("Moving Mouse"));
}

void AChessPlayerHand::LeftClickCallback()
{
    UE_LOG(LogTemp, Display, TEXT("Left clicked"));
}

void AChessPlayerHand::RightClickCallback()
{
	UE_LOG(LogTemp, Display, TEXT("Right clicked"));
}
