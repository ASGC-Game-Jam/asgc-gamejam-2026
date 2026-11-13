// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"

class UWorld;
class FAtlantisNavGraph;

/**
 * Debug drawing for the swimming nav graph, set with the Atlantis.AI.NavGraph.Draw console variable:
 * 1 = each AI's current path, 2 = nodes only, 3 = nodes and links near the camera (Atlantis.AI.NavGraph.DrawRadius).
 */
namespace Atlantis::AI::NavGraphDebug
{
	/** True while Atlantis.AI.NavGraph.Draw is above 0. */
	bool IsDrawEnabled();

	/** Draws the view selected by Atlantis.AI.NavGraph.Draw for one frame. */
	void DrawGraph(const UWorld& World, const FAtlantisNavGraph& Graph);
}
