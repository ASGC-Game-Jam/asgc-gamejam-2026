#include "GameplayDebugger.h"
#include "AI/GameplayDebuggerCategory_EnemyAI.h"
#include "Modules/ModuleManager.h"

#include "Audio/MetaSound/Interfaces/AtlantisAudioInterfaceRegistration.h"

class FProjectAtlantisModule : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override
	{
		ProjectAtlantisAudio::RegisterAllMetaSoundInterfaces();
		
#if WITH_GAMEPLAY_DEBUGGER
		IGameplayDebugger& GameplayDebuggerModule = IGameplayDebugger::Get();
		
		GameplayDebuggerModule.RegisterCategory(
			"AtlantisAI",
			IGameplayDebugger::FOnGetCategory::CreateStatic(&FGameplayDebuggerCategory_EnemyAI::MakeInstance),
			EGameplayDebuggerCategoryState::EnabledInGameAndSimulate
			);
#endif //WITH_GAMEPLAY_DEBUGGER
		
		// This optional module is selected by target rules, rather than auto-loaded via .uproject.
#if WITH_SCREENSHOT_CAPTURE
		FModuleManager::LoadModuleChecked<IModuleInterface>("ScreenshotCapture");
#endif

	}
};

	void ShutdownModule()
	{
#if WITH_GAMEPLAY_DEBUGGER
		if (IGameplayDebugger::IsAvailable())
		{
			IGameplayDebugger& GameplayDebuggerModule = IGameplayDebugger::Get();
			GameplayDebuggerModule.UnregisterCategory("AtlantisAI");
			GameplayDebuggerModule.NotifyCategoriesChanged();
		}
#endif //WITH_GAMEPLAY_DEBUGGER
	}


IMPLEMENT_PRIMARY_GAME_MODULE(
	FProjectAtlantisModule,
    ProjectAtlantis,
    "ProjectAtlantis"
);
