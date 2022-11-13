// Fill out your copyright notice in the Description page of Project Settings.


#include "VRCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Math/UnrealMathUtility.h"
#include "TimerManager.h"
#include "Components/CapsuleComponent.h"
#include "NavigationSystem.h"
#include "Components/PostProcessComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "MotionControllerComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "HandController.h"

// Sets default values
AVRCharacter::AVRCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	VRRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VRRoot"));
	//SetRootComponent(VRRoot);
	VRRoot->SetupAttachment(GetRootComponent());

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(VRRoot);

	TeleportPath = CreateDefaultSubobject<USplineComponent>(TEXT("TeleportPath"));
	TeleportPath->SetupAttachment(VRRoot);

	DestinationMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DestinationMarker"));
	DestinationMarker->SetupAttachment(GetRootComponent());

	PostProcessingComponent = CreateDefaultSubobject<UPostProcessComponent>(TEXT("PostProcessingComponent"));
	PostProcessingComponent->SetupAttachment(GetRootComponent());
}

// Called when the game starts or when spawned
void AVRCharacter::BeginPlay()
{
	Super::BeginPlay();

	//DestinationMarker->SetVisibility(false);

	//if (BlinkerMaterialBase != nullptr) {
	//	BlinkerMaterialInstance = UMaterialInstanceDynamic::Create(BlinkerMaterialBase, this);
	//	PostProcessingComponent->AddOrUpdateBlendable(BlinkerMaterialInstance);
	//}

	LeftController = GetWorld()->SpawnActor<AHandController>(HandControllerClass);
	if (LeftController != nullptr) {
		LeftController->SetHand(EControllerHand::Left);
		LeftController->SetOwner(this);
		LeftController->AttachToComponent(VRRoot, FAttachmentTransformRules::KeepRelativeTransform);
		
	}

	RightController = GetWorld()->SpawnActor<AHandController>(HandControllerClass);
	if (RightController != nullptr) {
		RightController->SetHand(EControllerHand::Right);
		RightController->SetOwner(this);
		RightController->AttachToComponent(VRRoot, FAttachmentTransformRules::KeepRelativeTransform);
	}

	// They set each other as other controller so that they can communicate, see each other
	LeftController->PairController(RightController);
}

// Called every frame
void AVRCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	FVector NewCameraOffset = Camera->GetComponentLocation() - GetActorLocation();
	NewCameraOffset.Z = 0;
	AddActorWorldOffset(-NewCameraOffset);
	VRRoot->AddWorldOffset(-NewCameraOffset);

	// UpdateDestinationMarker();
	//UpdateBlinkers();
}

void AVRCharacter::UpdateDestinationMarker()
{
	
	FVector Location;
	TArray<FVector> Path;
	bool bHasDestination = FindTeleportDestination(Path, Location);

	if (bHasDestination)
	{
		DestinationMarker->SetVisibility(true);
		//FRotator Rotation;
		//Rotation.Add(90, 0, 0);
		//DestinationMarker->SetWorldRotation(Rotation);
		DestinationMarker->SetWorldLocation(Location);
		DrawTeleportPath(Path);
	}
	else
	{
		DestinationMarker->SetVisibility(false);
		TArray<FVector> EmptyPath;
		DrawTeleportPath(EmptyPath);
	}
}

void AVRCharacter::UpdateBlinkers() {
	if (RadiusVsVelocity == nullptr) {
		return;
	}
	float Speed = GetVelocity().Size();
	float Radius = RadiusVsVelocity->GetFloatValue(Speed);

	BlinkerMaterialInstance->SetScalarParameterValue(TEXT("Radius"), Radius);
}

void AVRCharacter::DrawTeleportPath(TArray<FVector>&Path) {
	UpdateSpline(Path);

	for (USplineMeshComponent* DynamicMesh : TeleportPathMeshPool) {
		DynamicMesh->SetVisibility(false);
	}

	int32 SegmentNum = Path.Num() - 1;
	for (int32 i = 0; i < SegmentNum; ++i) {
		if (TeleportPathMeshPool.Num() <= i) {
			USplineMeshComponent* DynamicMesh = NewObject<USplineMeshComponent>(this);
			DynamicMesh->SetMobility(EComponentMobility::Movable);
			DynamicMesh->AttachToComponent(TeleportPath, FAttachmentTransformRules::KeepRelativeTransform);
			DynamicMesh->SetStaticMesh(TeleportArchMesh);
			DynamicMesh->SetMaterial(0, TeleportArchMaterial);
			DynamicMesh->RegisterComponent(); // for all dynamically created components it is  a must!!!
			TeleportPathMeshPool.Add(DynamicMesh);
		}
		USplineMeshComponent* DynamicMesh = TeleportPathMeshPool[i];
		DynamicMesh->SetVisibility(true);

		FVector StartPos, StartTangent, EndPos, EndTangent;
		TeleportPath->GetLocalLocationAndTangentAtSplinePoint(i, StartPos, StartTangent);
		TeleportPath->GetLocalLocationAndTangentAtSplinePoint(i + 1, EndPos, EndTangent);
		DynamicMesh->SetStartAndEnd(StartPos, StartTangent, EndPos, EndTangent);
	}
}

void AVRCharacter::UpdateSpline(TArray<FVector> &Path) {
	TeleportPath->ClearSplinePoints(false);
	for (int32 i = 0; i < Path.Num(); ++i) {
		FVector LocalPosition = TeleportPath->GetComponentTransform().InverseTransformPosition(Path[i]);
		FSplinePoint Point(i, LocalPosition, ESplinePointType::Curve);
		TeleportPath->AddPoint(Point, false);
	}
	TeleportPath->UpdateSpline();
}

bool AVRCharacter::FindTeleportDestination(TArray<FVector> &OutPath, FVector &OutLocation) {
	FVector Start = LeftController->GetActorLocation();
	//FVector Look = Start + LeftController->GetForwardVector() * MaxTeleportDistance;
	//Look = Look.RotateAngleAxis(30, LeftController->GetRightVector());
	//FVector End = Start + Start * MaxTeleportDistance;

	FVector Look = LeftController->GetActorForwardVector();

	FPredictProjectilePathParams Params(
		TeleportProjectileRadius, 
		Start, 
		Look * TeleportProjectileSpeed,
		TeleportSimulationTime, 
		ECollisionChannel::ECC_Visibility, 
		this
	);
	// Params.DrawDebugType = EDrawDebugTrace::ForOneFrame; // Draws debug Spheres
	Params.bTraceComplex = true; // if missing collision locations, hack
	FPredictProjectilePathResult Result;
	bool bHit = UGameplayStatics::PredictProjectilePath(this, Params, Result);

	//FHitResult HitResult;
	//bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility);

	if (!bHit) return false;

	for (FPredictProjectilePathPointData PointData : Result.PathData) {
		OutPath.Add(PointData.Location);
	}
	

	FNavLocation NavLocation;
	bool bOnNavMesh = UNavigationSystemV1::GetCurrent(GetWorld())->ProjectPointToNavigation(Result.HitResult.Location, NavLocation, TeleportProjectionExtent);

	if (!bOnNavMesh) return false;

	OutLocation = NavLocation.Location;

	return true;
}

// Called to bind functionality to input
void AVRCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis(TEXT("MoveLeft_Y"), this, &AVRCharacter::MoveForward);
	PlayerInputComponent->BindAxis(TEXT("MoveLeft_X"), this, &AVRCharacter::MoveRight);
	PlayerInputComponent->BindAxis(TEXT("MoveRight_X"), this, &AVRCharacter::Turn);
	PlayerInputComponent->BindAction(TEXT("Teleport"), IE_Released, this, &AVRCharacter::BeginTeleport);

	PlayerInputComponent->BindAction(TEXT("GripLeft"), IE_Pressed, this, &AVRCharacter::GripLeft);
	PlayerInputComponent->BindAction(TEXT("GripLeft"), IE_Released, this, &AVRCharacter::ReleaseLeft);
	PlayerInputComponent->BindAction(TEXT("GripRight"), IE_Pressed, this, &AVRCharacter::GripRight);
	PlayerInputComponent->BindAction(TEXT("GripRight"), IE_Released, this, &AVRCharacter::ReleaseRight);
}

void AVRCharacter::MoveForward(float throttle) {
	if (throttle != 0) {
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, FString::Printf(TEXT("Move fw throttle %f"), throttle));
	}
	AddMovementInput(throttle * LeftController->GetActorForwardVector());
}

void AVRCharacter::MoveRight(float throttle) {
	if (throttle != 0) {
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, FString::Printf(TEXT("Move l throttle %f"), throttle));
	}
	AddMovementInput(throttle * LeftController->GetActorRightVector());
}

void AVRCharacter::StartFade(float FromAlpha, float ToAlpha) {
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC != nullptr) {
		PC->PlayerCameraManager->StartCameraFade(FromAlpha, ToAlpha, TeleportFadeTime, FLinearColor::Black);
	}
}

void AVRCharacter::BeginTeleport() {

	StartFade(0, 1);
	FTimerHandle Handle;
	GetWorldTimerManager().SetTimer(Handle, this, &AVRCharacter::FinishTeleport, TeleportFadeTime, false);
}

void AVRCharacter::FinishTeleport() {
	FVector Destination = DestinationMarker->GetComponentLocation();
	Destination += GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * GetActorUpVector();
	SetActorLocation(Destination);

	StartFade(1, 0);
}

void AVRCharacter::Turn(float throttle) {
	
	//if (GEngine && throttle != 0) {
		//GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, FString::Printf(TEXT("Turn throttle %f"), throttle));
	//	FRotator Rotation;
	//	Rotation.Add(0, 3*throttle, 0);
	//	VRRoot->AddWorldRotation(Rotation);
	
	//}

	if (throttle != 0) {
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, FString::Printf(TEXT("Turn throttle %f"), throttle));
		//AddControllerYawInput(throttle);
		//FRotator Rotation;
		//Rotation.Add(0, throttle * LeftController->GetActorRotation(), 0);
		VRRoot->AddWorldRotation(LeftController->GetActorRotation());
	}
}
