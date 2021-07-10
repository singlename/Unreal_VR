// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "HandController.h"
#include "VRCharacter.generated.h"

UCLASS()
class VRPROJECTEMPTY_API AVRCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AVRCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:

	void UpdateDestinationMarker();

	void MoveForward(float throttle);
	void MoveRight(float throttle);
	void GripLeft() { LeftController->Grip(); };
	void ReleaseLeft() { LeftController->Release(); };
	void GripRight() { RightController->Grip(); };
	void ReleaseRight() { RightController->Release(); };
	void Turn(float throttle);
	void BeginTeleport();
	void FinishTeleport();
	bool FindTeleportDestination(TArray<FVector>& OutPath, FVector &OutLocation);
	void StartFade(float FromAlpha, float ToAlpha);
	void UpdateBlinkers();
	void UpdateSpline(TArray<FVector> &Path);
	void DrawTeleportPath(TArray<FVector>& Path);


private:
	UPROPERTY(VisibleAnywhere)
	class UCameraComponent* Camera;

	UPROPERTY()
	//class UMotionControllerComponent* LeftController;
	AHandController* LeftController;

	UPROPERTY()
	//class UMotionControllerComponent* RightController;
	AHandController* RightController;

	UPROPERTY()
	class USceneComponent* VRRoot;

	UPROPERTY(VisibleAnywhere)
	class USplineComponent* TeleportPath;

	UPROPERTY(VisibleAnywhere)
	class UStaticMeshComponent* DestinationMarker;

	UPROPERTY()
	class UPostProcessComponent* PostProcessingComponent;

	UPROPERTY()
	//class UStaticMeshComponent* DynamicMesh;
	TArray<class USplineMeshComponent*> TeleportPathMeshPool;

	UPROPERTY() // for garbage collection
	class UMaterialInstanceDynamic* BlinkerMaterialInstance;

private: // Configuration parameters
	UPROPERTY(EditAnywhere)
	float MaxTeleportDistance = 1000;

	UPROPERTY(EditAnywhere)
	float TeleportProjectileRadius = 10;

	UPROPERTY(EditAnywhere)
	float TeleportProjectileSpeed = 800; //cm/s

	UPROPERTY(EditAnywhere)
	float TeleportSimulationTime = 1;

	UPROPERTY(EditAnywhere)
	float TeleportFadeTime = 1;

	UPROPERTY(EditAnywhere)
	int CameraRotationAngle = 15;

	UPROPERTY(EditAnywhere)
	FVector TeleportProjectionExtent = FVector(100, 100, 100);

	UPROPERTY(EditAnywhere)
	class UMaterialInterface* BlinkerMaterialBase;

	UPROPERTY(EditAnywhere)
	class UCurveFloat* RadiusVsVelocity;

	UPROPERTY(EditDefaultsOnly) // edit only in blueprint
	class UStaticMesh* TeleportArchMesh;

	UPROPERTY(EditDefaultsOnly) // edit only in blueprint
	class UMaterialInterface* TeleportArchMaterial;

	UPROPERTY(EditDefaultsOnly) // edit only in blueprint
	TSubclassOf<AHandController> HandControllerClass;
};

