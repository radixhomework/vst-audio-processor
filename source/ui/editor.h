//------------------------------------------------------------------------
// Editor (plugin-ui spec, tasks 5.1-5.5): resizable panel built
// programmatically in VSTGUI (design D6). The layout is authored at a base
// of 420x220 and scaled as a whole on resize; the aspect ratio (21:11) is
// locked via checkSizeConstraint with bounds 315x165..840x440. Knobs and
// buttons carry the parameter IDs as control tags and go through the host
// (performEdit), so every move is host automation. A ~30 ms timer syncs
// control states and readouts during automation playback.
//------------------------------------------------------------------------
#pragma once

#include "vstgui/vstgui.h"
#include "vstgui/lib/cvstguitimer.h"

#include "public.sdk/source/vst/vstguieditor.h"

#include <utility>
#include <vector>

namespace Radixhomework {

class Controller;

// The depth is fixed by the SDK/VSTGUI base classes (VSTGUIEditor ->
// EditorView -> ...), same as any VSTGUI-based editor (cpp:S110 accepted).
class Editor : public Steinberg::Vst::VSTGUIEditor, public VSTGUI::IControlListener // NOSONAR
{
public:
	explicit Editor (Steinberg::Vst::EditController* controller);

	bool PLUGIN_API open (void* parent, const VSTGUI::PlatformType& platformType) SMTG_OVERRIDE;
	void PLUGIN_API close () SMTG_OVERRIDE;

	//---from IPlugView: resizable panel (task 5.5)--------------------------
	Steinberg::tresult PLUGIN_API canResize () SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API onSize (Steinberg::ViewRect* newSize) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API checkSizeConstraint (Steinberg::ViewRect* rect) SMTG_OVERRIDE;

	//---from IControlListener (VSTGUI)--------------------------------------
	void valueChanged (VSTGUI::CControl* control) SMTG_OVERRIDE;
	void controlBeginEdit (VSTGUI::CControl* control) SMTG_OVERRIDE;
	void controlEndEdit (VSTGUI::CControl* control) SMTG_OVERRIDE;

private:
	// Creates the panel content inside the frame.
	void buildControls ();
	// Repositions every control proportionally to the scale factor
	// (1.0 = the authored 420x220 base layout).
	void relayout (double scale);
	// Pushes current parameter values into knobs/buttons/readouts (unless
	// the user is dragging that control).
	void syncFromParameters ();
	// Refreshes the gain/pan readout labels for the given values.
	void updateGainReadout (const VSTGUI::CControl* knob);
	void updatePanReadout (const VSTGUI::CControl* knob);

	Controller* pluginController ();

	VSTGUI::CTextLabel* gainReadout {nullptr};
	VSTGUI::CTextLabel* panReadout {nullptr};
	VSTGUI::SharedPointer<VSTGUI::CVSTGUITimer> syncTimer;

	// Authored base rects (and font sizes) captured while building, for
	// proportional scaling on resize.
	std::vector<std::pair<VSTGUI::CView*, VSTGUI::CRect>> baseLayout;
	std::vector<std::pair<VSTGUI::CTextLabel*, VSTGUI::CCoord>> baseLabelFonts;
	VSTGUI::CFontDesc scaledFont;
};

} // namespace Radixhomework
