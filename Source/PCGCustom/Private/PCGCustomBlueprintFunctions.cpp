// Copyright Roman K., Epic Games, Inc. All Rights Reserved.

#include "PCGCustomBlueprintFunctions.h"
#include "PCGSubsystem.h"

void UPCGCustomBlueprintFunctions::RefreshRuntimePCG(UPARAM(DisplayName = "") UPCGComponent* Component, bool Force, EPCGChangeType ChangeType) {

	if (Component && Component->IsManagedByRuntimeGenSystem()) {
		if (UPCGSubsystem* Subsystem = Component->GetSubsystem()) {
			if (Force) {
				Subsystem->FlushCache();
			}
			Subsystem->RefreshRuntimeGenComponent(Component, ChangeType);
		}
	}

}

void UPCGCustomBlueprintFunctions::SetGenerationTrigger(UPARAM(DisplayName = "PCG Component") UPCGComponent* PCGComponent, EPCGComponentGenerationTrigger GenerationTrigger) {
	
	if (PCGComponent)
		PCGComponent->GenerationTrigger = GenerationTrigger;
}


void UPCGCustomBlueprintFunctions::ModifyActor(AActor* Actor) {
#if WITH_EDITOR
	if (Actor)
		Actor->Modify();
#endif
}
