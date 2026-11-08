#include "GameplayDebugger.h"
#include "AI/GameplayDebuggerCategory_EnemyAI.h"
#include "Modules/ModuleManager.h"

#include "Audio/MetaSound/Interfaces/AtlantisAudioInterfaceRegistration.h"

#if PLATFORM_MAC && WITH_EDITOR
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace
{
	// A revoked debugger console prevents the embedded Python interpreter from
	// creating its standard streams. Leave working Xcode and terminal streams intact.
	void RepairUnavailableStandardStreams()
	{
		for (int Stream = STDIN_FILENO; Stream <= STDERR_FILENO; ++Stream)
		{
			struct stat StreamStat;
			if (fstat(Stream, &StreamStat) == 0 || errno != EBADF)
			{
				continue;
			}

			const int NullStream = open("/dev/null", Stream == STDIN_FILENO ? O_RDONLY : O_WRONLY);
			if (NullStream >= 0 && NullStream != Stream)
			{
				dup2(NullStream, Stream);
				close(NullStream);
			}
		}
	}
}
#endif

class FProjectAtlantisModule : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override
	{
#if PLATFORM_MAC && WITH_EDITOR
		RepairUnavailableStandardStreams();
#endif
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
