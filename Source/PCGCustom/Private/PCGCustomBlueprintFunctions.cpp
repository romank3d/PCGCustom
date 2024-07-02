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