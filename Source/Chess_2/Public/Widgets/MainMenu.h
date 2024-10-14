// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainMenu.generated.h"

/**
 * 
 */
UCLASS()
class CHESS_2_API UMainMenu : public UUserWidget
{
	GENERATED_BODY()

public: 
 
protected: 
 
private:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess="true"))
	class UTextBlock* GameTitle;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess="true"))
	class UButton* PlayButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess="true"))
	class UButton* SettingsButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess="true"))
	class UButton* ExitButton;

	
};
