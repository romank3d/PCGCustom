// Copyright Roman K., Epic Games, Inc. All Rights Reserved. 

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PCGComponent.h"
#include "PCGCustomBlueprintFunctions.generated.h"

UCLASS()
class PCGCUSTOM_API UPCGCustomBlueprintFunctions : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, category = "PCGC Blueprint Functions")
		static void RefreshRuntimePCG(UPARAM(DisplayName = "PCG Component") UPCGComponent* PCGComponent, bool Force, EPCGChangeType ChangeType = EPCGChangeType::Structural);

	UFUNCTION(BlueprintCallable, category = "PCGC Blueprint Functions")
		static void SetGenerationTrigger(UPARAM(DisplayName = "PCG Component") UPCGComponent* PCGComponent, EPCGComponentGenerationTrigger GenerationTrigger);

	UFUNCTION(BlueprintCallable, category = "PCGC Blueprint Functions")
		static void ModifyActor(AActor* Actor);
};
