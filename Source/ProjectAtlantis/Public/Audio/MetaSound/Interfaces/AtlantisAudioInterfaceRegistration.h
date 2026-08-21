// Copyright Your Studio. All Rights Reserved.

#pragma once

namespace ProjectAtlantisAudio
{
	// Single entry point that registers every MetaSound interface in the project.
	// Add a call inside the .cpp each time a new interface file is created.
	void RegisterAllMetaSoundInterfaces();
}