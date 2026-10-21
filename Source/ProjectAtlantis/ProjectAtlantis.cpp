#include "Modules/ModuleManager.h"

#include "Audio/MetaSound/Interfaces/AtlantisAudioInterfaceRegistration.h"

class FProjectAtlantisModule : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override
	{
		ProjectAtlantisAudio::RegisterAllMetaSoundInterfaces();
		// This optional module is selected by target rules, rather than auto-loaded via .uproject.
#if WITH_SCREENSHOT_CAPTURE
		FModuleManager::LoadModuleChecked<IModuleInterface>("ScreenshotCapture");
#endif
	}
};


IMPLEMENT_PRIMARY_GAME_MODULE(
	FProjectAtlantisModule,
    ProjectAtlantis,
    "ProjectAtlantis"
);
