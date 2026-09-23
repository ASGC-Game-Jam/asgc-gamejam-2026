#include "Modules/ModuleManager.h"

#include "Audio/MetaSound/Interfaces/AtlantisAudioInterfaceRegistration.h"

class FProjectAtlantisModule : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override
	{
		ProjectAtlantisAudio::RegisterAllMetaSoundInterfaces();
	}
};


IMPLEMENT_PRIMARY_GAME_MODULE(
	FProjectAtlantisModule,
    ProjectAtlantis,
    "ProjectAtlantis"
);