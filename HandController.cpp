// Fill out your copyright notice in the Description page of Project Settings.


#include "HandController.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MotionControllerComponent.h"

// Sets default values
AHandController::AHandController()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	MotionController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("MotionController"));
	SetRootComponent(MotionController);


}

// Called when the game starts or when spawned
void AHandController::BeginPlay()
{
	Super::BeginPlay();

	OnActorBeginOverlap.AddDynamic(this, &AHandController::ActorBeginOverlap);
	OnActorEndOverlap.AddDynamic(this, &AHandController::ActorEndOverlap);
	
}

// Called every frame
void AHandController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsClimbing) {
		FVector HandControllerDelta = GetActorLocation() - ClimbingStartLocation;
		GetAttachParentActor()->AddActorWorldOffset(-HandControllerDelta);
	}

	if (bIsHolding) {
		// Hold object
	}
}

void AHandController::ActorBeginOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
	bool bNewCanClimb = CanClimb();
	if (!bCanClimb && bNewCanClimb)
	{
		UE_LOG(LogTemp, Warning, TEXT("Can Climb!"));
		APawn* Pawn = Cast<APawn>(GetAttachParentActor());
		if (Pawn != nullptr) {
			APlayerController* Controller = Cast<APlayerController>(Pawn->GetController());
			if (Controller != nullptr) {
				Controller->PlayHapticEffect(HapticEffect, MotionController->GetTrackingSource());
			}
		}
	}
	bCanClimb = bNewCanClimb;

	//bool bNewCanHold = CanHold();
	//if (!bCanHold && bNewCanHold)
	//{
		//UE_LOG(LogTemp, Warning, TEXT("Can Hold!"));
		//APawn* Pawn = Cast<APawn>(GetAttachParentActor());
		//if (Pawn != nullptr) {
		//	APlayerController* Controller = Cast<APlayerController>(Pawn->GetController());
		//	if (Controller != nullptr) {
		//		Controller->PlayHapticEffect(HapticEffect, MotionController->GetTrackingSource());
		//	}
		//}
	//}
	//bCanHold = bNewCanHold;
}

void AHandController::ActorEndOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
	bCanClimb = CanClimb();
	//bCanHold = CanHold();
}

bool AHandController::CanClimb() const
{
	TArray<AActor*> OverlappingActors;
	GetOverlappingActors(OverlappingActors);
	for (AActor* OverlappingActor : OverlappingActors)
	{
		if (OverlappingActor->ActorHasTag(TEXT("Climable")))
		{
			return true;
		}
	}

	return false;
}

bool AHandController::CanHold() const
{
	TArray<AActor*> OverlappingActors;
	GetOverlappingActors(OverlappingActors);
	for (AActor* OverlappingActor : OverlappingActors)
	{
		if (OverlappingActor->ActorHasTag(TEXT("Holdable")))
		{
			return true;
		}
	}

	return false;
}

void AHandController::Grip() {
	if (bCanClimb) {
		if (!bIsClimbing) {
			bIsClimbing = true;
			ClimbingStartLocation = GetActorLocation();

			// Set other controller as not climbing (steal controll)
			OtherController->bIsClimbing = false;

			ACharacter* Character = Cast<ACharacter>(GetAttachParentActor());
			if (Character != nullptr) {
				UE_LOG(LogTemp, Warning, TEXT("Climbing!"));
				Character->GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Flying);
			}
		}
	}

	if (bCanHold) {
		if (!bIsHolding) {
			bIsHolding = true;
			//HoldingStartLocation = GetActorLocation();

			// Set other controller as not climbing (steal controll)
			//OtherController->bIsHolding = false;

			//ACharacter* Character = Cast<ACharacter>(GetAttachParentActor());
			//if (Character != nullptr) {
				//UE_LOG(LogTemp, Warning, TEXT("Holding!"));
				// attach object to controller, assess position
				// Character->GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Flying);
			//}
		}
	}
}

void AHandController::Release() {
	if (bIsClimbing) {
		bIsClimbing = false;

		ACharacter* Character = Cast<ACharacter>(GetAttachParentActor());
		if (Character != nullptr) {
			Character->GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Falling);
		}
	}

	if (bIsHolding) {
		bIsHolding = false;

		//ACharacter* Character = Cast<ACharacter>(GetAttachParentActor());
		//if (Character != nullptr) {
			// make object fall down
			// Character->GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Falling);
		//}
	}
}

void AHandController::PairController(AHandController* Controller) {
	OtherController = Controller;
	OtherController->OtherController = this;
}
