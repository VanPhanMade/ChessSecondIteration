// Copyright (©) 2024, Van Phan. All rights reserved.


#include "Pawns/PlayablePawn.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameStates/ChessGameState.h"
#include "Camera/CameraComponent.h"
#include "Materials/MaterialInterface.h"
#include "GameModes/ChessGameMode.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "CollisionQueryParams.h"
#include "DrawDebugHelpers.h"
#include "Actors/BaseChessPiece.h"
#include "Actors/BoardSquare.h"
#include "PhysicsEngine/PhysicsHandleComponent.h"
#include "Net/UnrealNetwork.h"

APlayablePawn::APlayablePawn()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	if (!RootComponent) RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

	HandMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Hand Mesh"));
	HandMesh->SetupAttachment(RootComponent);

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Spring Arm"));
	SpringArm->SetupAttachment(RootComponent);

	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("Player Camera"));
	CameraComponent->SetupAttachment(SpringArm);

	PhysicsHandle = CreateDefaultSubobject<UPhysicsHandleComponent>(TEXT("Physics Handle"));
	PhysicsHandle->LinearStiffness = 3000.0f;
	PhysicsHandle->AngularStiffness = 10000.0f; 
	PhysicsHandle->LinearDamping = 200.0f;
	PhysicsHandle->AngularDamping = 3000.0f;  
	PhysicsHandle->SetIsReplicated(true);

	// Check if not a Class Default Object
    if (!HasAnyFlags(RF_ClassDefaultObject) && HandMaterial) 
    {
        HandMesh->SetMaterial(0, HandMaterial);
    }
}

void APlayablePawn::BeginPlay()
{
	Super::BeginPlay();

	if(HandMaterial) HandMesh->SetMaterial(0, HandMaterial);

	AChessGameMode* GameMode = Cast<AChessGameMode>(GetWorld()->GetAuthGameMode());
	if(GameMode)
	{
		GameMode->AddPlayablePawn(this);
	}

}

void APlayablePawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// if(const AChessGameState* GameState = Cast<AChessGameState>(GetWorld()->GetGameState<AGameStateBase>()))
	// {
	// 	if(GameState->CurrentTeamTurn != PlayerChessTeam) 
	// 	{
	// 		if(LastHitPiece) 
	// 		{
	// 			LastHitPiece->GetCurrentBoardSquare()->HighlightSquare(false);
	// 			LastHitPiece = nullptr;
	// 		}
	// 		HeldPiece = nullptr;
	// 		return;
	// 	}
	// }

	if(HeldPiece)
	{
		FVector TargetLocation = HandMesh->GetComponentLocation() + (GetActorForwardVector() * 180.0f) + (GetActorRightVector() * 20.0f);
		PhysicsHandle->SetTargetLocation(TargetLocation);
	}

	FVector CursorLocation;
	if(APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		FHitResult Hit;
		bool bHitSuccessful = PlayerController->GetHitResultUnderCursor(ECollisionChannel::ECC_GameTraceChannel1, true, Hit);
		if(bHitSuccessful)
		{
			CursorLocation = Hit.Location;
			CursorLocation.Z = 500.0f;

			FVector HandLocation = HandMesh->GetComponentLocation();
        	float DistanceToCursor = FVector::Dist(HandLocation, CursorLocation);

			if(DistanceToCursor <= 1000.0f) // ensures if we're hitting a location on the board
			{
        		FVector NewLocation = FMath::VInterpTo(HandLocation, CursorLocation, DeltaTime, 5.0f);
				HandMesh->SetWorldLocation(NewLocation + (GetActorForwardVector() * -20.0f));

				FHitResult HitResultUnderHand;  
				FVector Start = CursorLocation;
				FVector End = Start - FVector(0, 0, 500.0f);
				FCollisionQueryParams TraceParams;

				if(HeldPiece) return;
				if (GetWorld()->LineTraceSingleByChannel( HitResultUnderHand, Start, End, ECC_GameTraceChannel1, TraceParams))
				{
					if(ABoardSquare* HitSquare = Cast<ABoardSquare>(HitResultUnderHand.GetActor()))
					{
						AChessGameState* GameState = Cast<AChessGameState>(GetWorld()->GetGameState<AGameStateBase>());
						
						// Checks if we're targeting a new piece compared to last frame
						if(HitSquare->GetOccupant() != nullptr && HitSquare->GetOccupant()->GetChessTeam() == PlayerChessTeam && GameState->CurrentTeamTurn == PlayerChessTeam)
						{
							if(HitSquare->GetOccupant() != LastHitPiece)
							{
								if(LastHitPiece) LastHitPiece->HighlightSquare(false);
								LastHitPiece = HitSquare->GetOccupant();
								LastHitPiece->HighlightSquare(true);
							}
						}
						else
						{
							if(LastHitPiece) LastHitPiece->HighlightSquare(false);
							LastHitPiece = nullptr;
							HitSquare->HighlightSquare(false);
						}
					}
				}
			}
		}
	}
}

void APlayablePawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(MovementAction, ETriggerEvent::Triggered, this, &ThisClass::MoveCallback);
		EnhancedInputComponent->BindAction(MouseAction, ETriggerEvent::Triggered, this, &ThisClass::MouseCallback);
		EnhancedInputComponent->BindAction(LeftClickAction, ETriggerEvent::Started, this, &ThisClass::LeftClickCallback);
		EnhancedInputComponent->BindAction(RightClickAction, ETriggerEvent::Started, this, &ThisClass::RightClickCallback);
	}
}

void APlayablePawn::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(APlayablePawn, PhysicsHandle);
    DOREPLIFETIME(APlayablePawn, LastHitPiece);
    DOREPLIFETIME(APlayablePawn, HeldPiece);
}

void APlayablePawn::MoveCallback(const FInputActionValue &Value)
{
	FVector2D InputVector = Value.Get<FVector2D>();
	FVector Direction = (GetActorForwardVector() * InputVector.Y * 5.0f) + (GetActorRightVector() * InputVector.X * 5.0f);
	SetActorLocation(GetActorLocation() + Direction);
}

void APlayablePawn::MouseCallback(const FInputActionValue &Value)
{

}

void APlayablePawn::LeftClickCallback()
{
	if(HeldPiece)
	{
		if(APlayerController* PlayerController = Cast<APlayerController>(GetController()))
		{
			FHitResult Hit;
			bool bHitSuccessful = PlayerController->GetHitResultUnderCursor(ECollisionChannel::ECC_GameTraceChannel2, true, Hit);
			if(bHitSuccessful)
			{
				PhysicsHandle->ReleaseComponent();
				if(ABoardSquare* HitSquare = Cast<ABoardSquare>(Hit.GetActor())) LastHitPiece->MoveChessPiece(HitSquare);
				if(ABaseChessPiece* HitPiece = Cast<ABaseChessPiece>(Hit.GetActor())) LastHitPiece->MoveChessPiece(HitPiece->GetCurrentBoardSquare());
				if(!HasAuthority())
				{
					Server_PlacePiece(Hit, LastHitPiece);
				}
				HeldPiece = nullptr;
				LastHitPiece = nullptr;
			}
		}
		
	}
	else{
		if(LastHitPiece)
		{
			HeldPiece = LastHitPiece;
			UPrimitiveComponent* PieceRoot = Cast<UPrimitiveComponent>(HeldPiece->GetRootComponent());
			HeldPiece->SetActorEnableCollision(false);
			// Pickup piece
			if (PieceRoot && PieceRoot->IsSimulatingPhysics())
    		{
        		FVector HandLocation = HandMesh->GetComponentLocation();
        		PhysicsHandle->GrabComponentAtLocation(PieceRoot, NAME_None, (HeldPiece->GetActorLocation() + FVector(0,0, 100.0f)));
				HeldPiece->PickupChessPiece();
    		}

		}
	}
}

void APlayablePawn::RightClickCallback()
{
	if(HeldPiece)
	{
		PhysicsHandle->ReleaseComponent();
		HeldPiece->DropChessPiece();
		HeldPiece = nullptr;
	}
}

void APlayablePawn::OnTurnChange()
{
	if(const AChessGameState* GameState = Cast<AChessGameState>(GetWorld()->GetGameState<AGameStateBase>()))
	if(GameState->CurrentTeamTurn != PlayerChessTeam)
    {
        if(LastHitPiece)
        {
			if(ABoardSquare* CurrentPieceBoard = LastHitPiece->GetCurrentBoardSquare())
			{
				CurrentPieceBoard->HighlightSquare(false);
			}
            
            LastHitPiece = nullptr;
        }
		if(HeldPiece)
		{
			if(ABoardSquare* CurrentPieceBoard = HeldPiece->GetCurrentBoardSquare())
			{
				CurrentPieceBoard->HighlightSquare(false);
			}
			
			HeldPiece = nullptr;
		}

    }
	else
	{
		if(LastHitPiece)
        {
			if(ABoardSquare* CurrentPieceBoard = LastHitPiece->GetCurrentBoardSquare())
			{
				CurrentPieceBoard->HighlightSquare(false);
			}
            
            LastHitPiece = nullptr;
        }
		if(HeldPiece)
		{
			if(ABoardSquare* CurrentPieceBoard = HeldPiece->GetCurrentBoardSquare())
			{
				CurrentPieceBoard->HighlightSquare(false);
			}
			
			HeldPiece = nullptr;
		}

	}
}

void APlayablePawn::Server_PlacePiece_Implementation(FHitResult DesiredPlacementHitData, ABaseChessPiece* LastHitPieceFromClient)
{
	if(!DesiredPlacementHitData.IsValidBlockingHit() || !LastHitPieceFromClient) return;
	if(const AChessGameState* GameState = Cast<AChessGameState>(GetWorld()->GetGameState<AGameStateBase>()))
	{
		if(GameState->CurrentTeamTurn != PlayerChessTeam) return;
	}

	if(ABoardSquare* HitSquare = Cast<ABoardSquare>(DesiredPlacementHitData.GetActor())) LastHitPieceFromClient->MoveChessPiece(HitSquare);
	if(ABaseChessPiece* HitPiece = Cast<ABaseChessPiece>(DesiredPlacementHitData.GetActor())) LastHitPieceFromClient->MoveChessPiece(HitPiece->GetCurrentBoardSquare());
	HeldPiece = nullptr;
	LastHitPiece = nullptr;
}
