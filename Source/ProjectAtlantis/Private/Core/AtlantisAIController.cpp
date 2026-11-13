#include "Core/AtlantisAIController.h"

#include "AI/Navigation/AtlantisPathFollowingComponent.h"
#include "Components/StateTreeAIComponent.h"

AAtlantisAIController::AAtlantisAIController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UAtlantisPathFollowingComponent>(TEXT("PathFollowingComponent")))
{
	StateTreeComponent = CreateDefaultSubobject<UStateTreeAIComponent>("State Tree");
}
