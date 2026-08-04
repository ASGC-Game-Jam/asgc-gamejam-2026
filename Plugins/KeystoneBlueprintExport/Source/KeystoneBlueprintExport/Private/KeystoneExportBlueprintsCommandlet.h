// Headless export — the "artists do nothing" path. Keystone (or CI) runs this on a build
// agent that has the engine + a checkout of the project:
//
//   UnrealEditor-Cmd <Project>.uproject -run=KeystoneExportBlueprints [-commit] [-path=/Game/...]
//
// It exports every Blueprint to `.bpgraph.json` and, with -commit, stages+commits+pushes just
// the BlueprintGraphs/ folder. No editor UI, no artist involvement.
#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "KeystoneExportBlueprintsCommandlet.generated.h"

UCLASS()
class UKeystoneExportBlueprintsCommandlet : public UCommandlet
{
    GENERATED_BODY()

public:
    virtual int32 Main(const FString& Params) override;
};
