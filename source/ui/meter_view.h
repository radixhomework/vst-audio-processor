//------------------------------------------------------------------------
// MeterView (plugin-ui spec, task 5.4): one vertical meter bar per output
// channel — RMS fill, instantaneous peak line, falling peak-hold (~2 s)
// against a dB scale (0/-6/-12/-24/-48), plus a clickable clip LED. Polls
// the controller's meter cache on a ~30 ms VSTGUI timer.
//------------------------------------------------------------------------
#pragma once

#include "vstgui/vstgui.h"

namespace Radixhomework {

class Controller;

class MeterView : public VSTGUI::CView
{
public:
	MeterView (const VSTGUI::CRect& size, Controller& controller, int channel);
	~MeterView () override;

	void draw (VSTGUI::CDrawContext* context) override;
	VSTGUI::CMouseEventResult onMouseDown (VSTGUI::CPoint& where,
	                                       const VSTGUI::CButtonState& buttons) override;

private:
	// Maps a linear level to the y position inside the bar rect.
	VSTGUI::CCoord levelToY (float linear) const;
	VSTGUI::CRect barRect () const;
	VSTGUI::CRect ledRect () const;

	Controller& controller;
	int channel;

	// View-side peak-hold state (resets when the editor reopens — accepted
	// by design D5).
	float holdLevel {0.f};
	double holdAgeSeconds {0.};
	bool holdActive {false};

	VSTGUI::SharedPointer<VSTGUI::CVSTGUITimer> timer;
};

} // namespace Radixhomework
