//------------------------------------------------------------------------
// Editor (plugin-ui spec, tasks 5.1-5.3): fixed ~420x220 panel built
// programmatically in VSTGUI (design D6). Knobs and buttons carry the
// parameter IDs as control tags and go through the host (performEdit), so
// every move is host automation. A ~30 ms timer syncs control states and
// readouts during automation playback.
//------------------------------------------------------------------------
#pragma once

#include "vstgui/vstgui.h"
#include "vstgui/lib/cvstguitimer.h"

#include "public.sdk/source/vst/vstguieditor.h"

namespace Radixhomework {

class Controller;

class Editor : public Steinberg::Vst::VSTGUIEditor, public VSTGUI::IControlListener
{
public:
	explicit Editor (Steinberg::Vst::EditController* controller);

	bool PLUGIN_API open (void* parent, const VSTGUI::PlatformType& platformType) SMTG_OVERRIDE;
	void PLUGIN_API close () SMTG_OVERRIDE;

	//---from IControlListener (VSTGUI)------------------------------------
	void valueChanged (VSTGUI::CControl* control) SMTG_OVERRIDE;
	void controlBeginEdit (VSTGUI::CControl* control) SMTG_OVERRIDE;
	void controlEndEdit (VSTGUI::CControl* control) SMTG_OVERRIDE;

private:
	// Creates the panel content inside the frame.
	void buildControls ();
	// Pushes current parameter values into knobs/buttons/readouts (unless
	// the user is dragging that control).
	void syncFromParameters ();
	// Refreshes the gain/pan readout labels for the given values.
	void updateGainReadout (VSTGUI::CControl* knob);
	void updatePanReadout (VSTGUI::CControl* knob);

	Controller* pluginController ();

	VSTGUI::CTextLabel* gainReadout {nullptr};
	VSTGUI::CTextLabel* panReadout {nullptr};
	VSTGUI::SharedPointer<VSTGUI::CVSTGUITimer> syncTimer;
};

} // namespace Radixhomework
