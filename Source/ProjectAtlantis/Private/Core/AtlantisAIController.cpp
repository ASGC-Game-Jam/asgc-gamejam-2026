#include "Core/AtlantisAIController.h"

#include "Components/StateTreeAIComponent.h"

AAtlantisAIController::AAtlantisAIController()
{
	StateTreeComponent = CreateDefaultSubobject<UStateTreeAIComponent>("State Tree");
}
