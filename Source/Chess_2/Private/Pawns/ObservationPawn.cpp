// Copyright (©) 2024, Van Phan. All rights reserved.


#include "Pawns/ObservationPawn.h"
#include "GameFramework/SpringArmComponent.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

AObservationPawn::AObservationPawn()
{
	PrimaryActorTick.bCanEverTick = true;
	if (!RootComponent) RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Spring Arm"));
	SpringArm->TargetArmLength = 0.0f;
	SpringArm->bUsePawnControlRotation = true;
}

void AObservationPawn::BeginPlay()
{
	Super::BeginPlay();

	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			UE_LOG(LogTemp, Display, TEXT("Subsystem found, adding mapping context"));
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Subsystem not found"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayerController not found"));
	}
	
}

void AObservationPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AObservationPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
    {
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ThisClass::LookActionCall);
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ThisClass::MoveActionCall);
		EnhancedInputComponent->BindAction(PrimaryAction, ETriggerEvent::Triggered, this, &ThisClass::PrimaryActionCall);
		EnhancedInputComponent->BindAction(CancelPrimaryAction, ETriggerEvent::Triggered, this, &ThisClass::CancelPrimaryActionCall);
		EnhancedInputComponent->BindAction(OpenUIAction, ETriggerEvent::Triggered, this, &ThisClass::OpenUIActionCall);
    }
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to cast PlayerInputComponent to UEnhancedInputComponent."));
	}

}

void AObservationPawn::LookActionCall(const FInputActionValue &Value)
{

}

void AObservationPawn::MoveActionCall(const FInputActionValue &Value)
{

}

void AObservationPawn::PrimaryActionCall()
{

}

void AObservationPawn::CancelPrimaryActionCall()
{

}

void AObservationPawn::OpenUIActionCall()
{

}
