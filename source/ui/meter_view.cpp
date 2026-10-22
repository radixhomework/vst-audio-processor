#include "meter_view.h"

#include "controller.h"

#include "vstgui/lib/cvstguitimer.h"

#include <array>
#include <chrono>
#include <cmath>
#include <format>

using namespace VSTGUI;

namespace Radixhomework {

//------------------------------------------------------------------------
namespace {

constexpr float kMeterFloorDb = -60.f;
constexpr float kHoldTimeSeconds = 2.f;
constexpr float kHoldFallDbPerSecond = 60.f;

float linearToDb (float linear)
{
	return (linear > 0.f) ? 20.f * std::log10 (linear) : kMeterFloorDb;
}

const CColor backgroundColor {50, 52, 58};
const CColor barBackgroundColor {28, 30, 34};
const CColor rmsColor {96, 200, 96};
const CColor peakColor {230, 230, 230};
const CColor holdColor {240, 90, 90};
const CColor scaleColor {140, 142, 148};
const CColor clipLitColor {240, 60, 60};
const CColor clipDarkColor {70, 30, 30};

constexpr std::array<int, 5> kScaleMarksDb {0, -6, -12, -24, -48};

} // namespace

//------------------------------------------------------------------------
MeterView::MeterView (const CRect& size, Controller& ctrl, int ch)
: CView (size), controller (ctrl), channel (ch)
{
	// Constructor call: explicitly the CView implementation.
	CView::setWantsFocus (false);
	// ~30 ms poll (plugin-ui spec).
	timer = makeOwned<CVSTGUITimer> (
	    [this] (CVSTGUITimer*) {
		    invalid ();
	    },
	    30);
}

MeterView::~MeterView () = default;

//------------------------------------------------------------------------
CRect MeterView::barRect () const
{
	auto bounds = getViewSize ();
	return CRect (bounds.left, bounds.top + 16, bounds.right, bounds.bottom);
}

//------------------------------------------------------------------------
CRect MeterView::ledRect () const
{
	auto bounds = getViewSize ();
	return CRect (bounds.left + 1, bounds.top + 1, bounds.right - 1, bounds.top + 13);
}

//------------------------------------------------------------------------
CCoord MeterView::levelToY (float linear) const
{
	const CRect bar = barRect ();
	const float db = std::max (linearToDb (linear), kMeterFloorDb);
	const float normalized = 1.f + db / -kMeterFloorDb; // 0 at floor, 1 at 0 dBFS
	return bar.bottom - normalized * bar.getHeight ();
}

//------------------------------------------------------------------------
void MeterView::draw (CDrawContext* context)
{
	auto bounds = getViewSize ();
	CDrawContext::Transform t (*context,
	                           CGraphicsTransform ().translate (bounds.getTopLeft ()));
	bounds.originize ();

	// Background + clip LED.
	context->setFillColor (backgroundColor);
	context->drawRect (bounds, kDrawFilled);
	const bool clipped = controller.meterCache ().getClip (channel);
	context->setFillColor (clipped ? clipLitColor : clipDarkColor);
	context->drawRect (ledRect (), kDrawFilled);

	// Bar background.
	const CRect bar = barRect ();
	context->setFillColor (barBackgroundColor);
	context->drawRect (bar, kDrawFilled);

	// Current values from the controller cache.
	const float rms = controller.meterCache ().getRms (channel);
	const float peak = controller.meterCache ().getPeak (channel);

	// Peak-hold ballistics (view-side): hold the highest peak ~2 s, then
	// fall toward the current level.
	const float tickSeconds = 0.03f;
	if (peak >= holdLevel || !holdActive)
	{
		holdLevel = peak;
		holdAgeSeconds = 0.;
		holdActive = true;
	}
	else
	{
		holdAgeSeconds += tickSeconds;
		if (holdAgeSeconds > kHoldTimeSeconds)
		{
			const float fall = std::pow (10.f, -kHoldFallDbPerSecond * tickSeconds / 20.f);
			holdLevel *= fall;
			if (holdLevel < peak)
			{
				holdLevel = peak;
				holdActive = false;
			}
		}
	}

	// RMS fill.
	const CCoord rmsY = levelToY (rms);
	context->setFillColor (rmsColor);
	context->drawRect (CRect (bar.left, rmsY, bar.right, bar.bottom), kDrawFilled);

	// Peak line.
	context->setFrameColor (peakColor);
	context->setLineWidth (2.);
	const CCoord peakY = levelToY (peak);
	context->drawLine (CPoint (bar.left, peakY), CPoint (bar.right, peakY));

	// Peak-hold marker.
	context->setFrameColor (holdColor);
	const CCoord holdY = levelToY (holdLevel);
	context->drawLine (CPoint (bar.left, holdY), CPoint (bar.right, holdY));

	// dB scale ticks and labels.
	context->setFrameColor (scaleColor);
	context->setFontColor (scaleColor);
	context->setFont (kSystemFont, 9.);
	for (int db : kScaleMarksDb)
	{
		const float linear = std::pow (10.f, static_cast<float> (db) / 20.f);
		const CCoord y = levelToY (linear);
		context->drawLine (CPoint (bar.right, y), CPoint (bar.right + 3, y));
		std::array<char, 8> label {};
		std::format_to_n (label.data (), label.size () - 1, "{}", db);
		context->drawString (label.data (), CPoint (bar.right + 5, y + 3));
	}

	setDirty (false);
}

//------------------------------------------------------------------------
CMouseEventResult MeterView::onMouseDown (CPoint& where, const CButtonState& buttons)
{
	// Clicking the clip LED resets the latched flag (completes task 4.2);
	// where is in frame coordinates, as is getViewSize().
	if (buttons.isLeftButton () && ledRect ().pointInside (where) &&
	    controller.meterCache ().getClip (channel))
	{
		controller.requestClipReset (channel);
		invalid ();
	}
	return kMouseDownEventHandledButDontNeedMovedOrUpEvents;
}

} // namespace Radixhomework
