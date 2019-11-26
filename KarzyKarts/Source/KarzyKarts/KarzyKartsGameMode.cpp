// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "KarzyKartsGameMode.h"
#include "KarzyKartsPawn.h"
#include "KarzyKartsHud.h"

AKarzyKartsGameMode::AKarzyKartsGameMode()
{
	DefaultPawnClass = AKarzyKartsPawn::StaticClass();
	HUDClass = AKarzyKartsHud::StaticClass();
}
