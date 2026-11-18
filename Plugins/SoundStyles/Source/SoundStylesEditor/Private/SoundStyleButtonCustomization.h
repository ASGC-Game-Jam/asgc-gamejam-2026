// Copyright (c) 2026 clarasmith-sound
// SPDX-License-Identifier: MIT

#pragma once

#include "IDetailCustomization.h"
#include "Templates/SharedPointer.h"

class IDetailLayoutBuilder;

/**
 * Details panel for Sound Style Button: hides the Button Style's Hovered/Pressed/Clicked Sound slots,
 * because the Sound Styles table plays those sounds. (The button also clears them at runtime; see ClearStyleSounds.)
 */
class FSoundStyleButtonCustomization : public IDetailCustomization
{
public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance();

private:
	//~ Begin IDetailCustomization interface */
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
	//~ End IDetailCustomization interface */

};
