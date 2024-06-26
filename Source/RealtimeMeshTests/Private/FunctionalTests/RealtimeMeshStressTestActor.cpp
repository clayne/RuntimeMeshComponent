﻿// Copyright (c) 2015-2024 TriAxis Games, L.L.C. All Rights Reserved.


#include "FunctionalTests/RealtimeMeshStressTestActor.h"

#include "MaterialDomain.h"
#include "RealtimeMeshLibrary.h"
#include "RealtimeMeshSimple.h"
#include "Mesh/RealtimeMeshBasicShapeTools.h"
#include "Mesh/RealtimeMeshBuilder.h"


ARealtimeMeshStressTestActor::ARealtimeMeshStressTestActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

static const auto SectionGroupKey = FRealtimeMeshSectionGroupKey::Create(0, FName("TestTripleBox"));

void ARealtimeMeshStressTestActor::OnGenerateMesh_Implementation()
{
	Super::OnGenerateMesh_Implementation();

	// Initialize the simple mesh
	URealtimeMeshSimple* RealtimeMesh = GetRealtimeMeshComponent()->InitializeRealtimeMesh<URealtimeMeshSimple>();
	
	// Setup the two material slots
	RealtimeMesh->SetupMaterialSlot(0, "PrimaryMaterial", UMaterial::GetDefaultMaterial(EMaterialDomain::MD_Surface));
	RealtimeMesh->SetupMaterialSlot(1, "SecondaryMaterial", UMaterial::GetDefaultMaterial(EMaterialDomain::MD_Surface));

	FRealtimeMeshStreamSet StreamSet;	
	// Here we're using the builder just to initialize the stream set
	TRealtimeMeshBuilderLocal<uint16, FPackedNormal, FVector2DHalf, 2> Builder(StreamSet);
	Builder.EnableTangents();
	Builder.EnableTexCoords();
	Builder.EnableColors();
	Builder.EnablePolyGroups();

	URealtimeMeshBasicShapeTools::AppendBoxMesh(StreamSet, FVector3f(100.0f, 100.0f, 200.0f), FTransform3f::Identity, 2, FColor::White);
	URealtimeMeshBasicShapeTools::AppendBoxMesh(StreamSet, FVector3f(200.0f, 100.0f, 100.0f), FTransform3f::Identity, 1, FColor::White);
	URealtimeMeshBasicShapeTools::AppendBoxMesh(StreamSet, FVector3f(100.0f, 200.0f, 100.0f), FTransform3f::Identity, 3, FColor::White);

	RealtimeMesh->CreateSectionGroup(SectionGroupKey, StreamSet);

	RealtimeMesh->UpdateSectionConfig(FRealtimeMeshSectionKey::CreateForPolyGroup(SectionGroupKey, 1), FRealtimeMeshSectionConfig(ERealtimeMeshSectionDrawType::Static, 0));
	RealtimeMesh->UpdateSectionConfig(FRealtimeMeshSectionKey::CreateForPolyGroup(SectionGroupKey, 2), FRealtimeMeshSectionConfig(ERealtimeMeshSectionDrawType::Dynamic, 0));
	RealtimeMesh->UpdateSectionConfig(FRealtimeMeshSectionKey::CreateForPolyGroup(SectionGroupKey, 3), FRealtimeMeshSectionConfig(ERealtimeMeshSectionDrawType::Static, 1));
		
}

void ARealtimeMeshStressTestActor::TickActor(float DeltaTime, ELevelTick TickType, FActorTickFunction& ThisTickFunction)
{
	if (PendingGeneration.IsValid())
	{
		PendingGeneration.Wait();
		PendingGeneration.Reset();
	}
	
	if (URealtimeMeshSimple* RealtimeMesh = GetRealtimeMeshComponent()->GetRealtimeMeshAs<URealtimeMeshSimple>())
	{
		TSharedRef<TStrongObjectPtr<URealtimeMeshSimple>> RealtimeMeshRef = MakeShared<TStrongObjectPtr<URealtimeMeshSimple>>(RealtimeMesh);

		TPromise<TSharedPtr<TStrongObjectPtr<URealtimeMeshSimple>>> Promise;
		PendingGeneration = Promise.GetFuture();
		AsyncTask(ENamedThreads::AnyThread, [RealtimeMeshRef, Promise = MoveTemp(Promise)]() mutable
		{
			FRealtimeMeshStreamSet StreamSet;
			// Here we're using the builder just to initialize the stream set
			TRealtimeMeshBuilderLocal<uint16, FPackedNormal, FVector2DHalf, 2> Builder(StreamSet);
			Builder.EnableTangents();
			Builder.EnableTexCoords();
			Builder.EnableColors();
			Builder.EnablePolyGroups();

			const float Scale = (FMath::Cos(FPlatformTime::Seconds() * PI) * 0.25f) + 0.5f;
			const FTransform3f Transform = FTransform3f(FQuat4f::Identity, FVector3f(0, 0, 0), FVector3f(Scale, Scale, Scale));

			URealtimeMeshBasicShapeTools::AppendBoxMesh(StreamSet, FVector3f(100.0f, 100.0f, 200.0f), Transform, 2, FColor::White);
			URealtimeMeshBasicShapeTools::AppendBoxMesh(StreamSet, FVector3f(200.0f, 100.0f, 100.0f), Transform, 1, FColor::White);
			URealtimeMeshBasicShapeTools::AppendBoxMesh(StreamSet, FVector3f(100.0f, 200.0f, 100.0f), Transform, 3, FColor::White);

			RealtimeMeshRef.Get()->UpdateSectionGroup(SectionGroupKey, StreamSet);
	RealtimeMeshRef.Get()->SetupMaterialSlot(0, "PrimaryMaterial", UMaterial::GetDefaultMaterial(EMaterialDomain::MD_Surface));
	RealtimeMeshRef.Get()->SetupMaterialSlot(1, "SecondaryMaterial", UMaterial::GetDefaultMaterial(EMaterialDomain::MD_Surface));
			Promise.EmplaceValue(RealtimeMeshRef);
		});
		
		Super::TickActor(DeltaTime, TickType, ThisTickFunction);
	}
}
