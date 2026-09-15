#include "Core/AtlantisAIController.h"

#include "Components/StateTreeAIComponent.h"
#include "Perception/AIPerceptionComponent.h"

AAtlantisAIController::AAtlantisAIController()
{
	AIPerception = CreateDefaultSubobject<UAIPerceptionComponent>("AI Perception");
	StateTreeComponent = CreateDefaultSubobject<UStateTreeAIComponent>("State Tree");
}
