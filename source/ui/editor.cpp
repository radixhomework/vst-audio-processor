#include "editor.h"

#include "controller.h"
#include "meter_view.h"
#include "params.h"

#include <array>
#include <cmath>
#include <format>
#include <numbers>

using namespace VSTGUI;

namespace Radixhomework {

//------------------------------------------------------------------------
namespace {

const CColor panelColor {40, 44, 52};
const CColor textColor {220, 222, 228};

// Knob travel: 270 degrees, from 135 (lower-left) counter-clockwise to
// 45 (lower-right).
constexpr double kKnobStartAngleDeg = 135.;
constexpr double kKnobRangeDeg = 270.;

const CColor knobTrackColor {70, 74, 82};
const CColor knobValueColor {150, 200, 240};
const CColor knobPointerColor {230, 232, 238};

//------------------------------------------------------------------------
// Minimal vector knob: drag vertically to change; double-click resets to
// the default. No bitmaps — the panel is fully programmatic (design D6).
class VectorKnob : public CControl
{
public:
	VectorKnob (const CRect& size, IControlListener* listener, int32_t tag)
	: CControl (size, listener, tag)
	{
		// Constructor call: explicitly the CView implementation.
		CView::setWantsFocus (false);
	}

	void draw (CDrawContext* context) override
	{
		auto bounds = getViewSize ();
		CDrawContext::Transform t (*context,
		                           CGraphicsTransform ().translate (bounds.getTopLeft ()));
		bounds.originize ();

		const CCoord radius = std::min (bounds.getWidth (), bounds.getHeight ()) / 2 - 3;
		const CPoint center (bounds.getWidth () / 2, bounds.getHeight () / 2);
		const CRect arcRect (center.x - radius, center.y - radius, center.x + radius,
		                     center.y + radius);

		context->setDrawMode (kAntiAliasing);

		// Track arc.
		context->setFrameColor (knobTrackColor);
		context->setLineWidth (3.);
		context->drawArc (arcRect, static_cast<float> (kKnobStartAngleDeg),
		                  static_cast<float> (kKnobStartAngleDeg + kKnobRangeDeg));

		// Value arc.
		const double valueAngle =
		    kKnobStartAngleDeg + kKnobRangeDeg * static_cast<double> (getValue ());
		context->setFrameColor (knobValueColor);
		context->drawArc (arcRect, static_cast<float> (kKnobStartAngleDeg),
		                  static_cast<float> (valueAngle));

		// Pointer.
		const double radians = valueAngle * std::numbers::pi / 180.;
		const CPoint direction (std::cos (radians), -std::sin (radians));
		context->setFrameColor (knobPointerColor);
		context->setLineWidth (2.);
		context->drawLine (CPoint (center.x + direction.x * radius * 0.4,
		                           center.y + direction.y * radius * 0.4),
		                   CPoint (center.x + direction.x * radius, center.y + direction.y * radius));

		setDirty (false);
	}

	CMouseEventResult onMouseDown (CPoint& where, const CButtonState& buttons) override
	{
		if (buttons.isDoubleClick ())
		{
			beginEdit ();
			setValue (getDefaultValue ());
			valueChanged ();
			endEdit ();
			invalid ();
			return kMouseDownEventHandledButDontNeedMovedOrUpEvents;
		}
		if (buttons.isLeftButton ())
		{
			dragStart = where;
			dragStartValue = getValue ();
			beginEdit ();
			onMouseMoved (where, buttons);
			return kMouseEventHandled;
		}
		return kMouseEventNotHandled;
	}

	CMouseEventResult onMouseUp (CPoint& where, const CButtonState& buttons) override
	{
		if (isEditing ())
		{
			endEdit ();
			return kMouseEventHandled;
		}
		return kMouseEventNotHandled;
	}

	CMouseEventResult onMouseMoved (CPoint& where, const CButtonState& buttons) override
	{
		if (isEditing ())
		{
			const double delta =
			    (static_cast<double> (dragStart.y) - static_cast<double> (where.y)) * 0.005;
			setValue (static_cast<float> (std::clamp (dragStartValue + delta, 0., 1.)));
			invalid ();
			valueChanged ();
			return kMouseEventHandled;
		}
		return kMouseEventNotHandled;
	}

	CLASS_METHODS (VectorKnob, CControl)

private:
	CPoint dragStart;
	double dragStartValue {0.};
};

//------------------------------------------------------------------------
// VSTGUI containers own their views through reference counting once
// addView hands the pointer over; raw new is the framework's idiom
// (cpp:S5025 accepted here).
CTextLabel* addLabel (CViewContainer* container, const CRect& rect, const char* text,
                      CHoriTxtAlign alignment = kCenterText)
{
	auto* label = new CTextLabel (rect, text); // NOSONAR
	label->setTransparency (true);
	label->setFontColor (textColor);
	label->setHoriAlign (alignment);
	container->addView (label);
	return label;
}

} // namespace

//------------------------------------------------------------------------
namespace {

// The panel size must be known BEFORE the view is attached: hosts size
// their editor window from IPlugView::getSize() at creation, so the base
// class needs the rect right at construction. Fixed panel, shared by all
// instances (design D6).
Steinberg::ViewRect& panelViewRect ()
{
	static Steinberg::ViewRect rect (0, 0, 420, 220);
	return rect;
}

} // namespace

//------------------------------------------------------------------------
Editor::Editor (Steinberg::Vst::EditController* editController)
: Steinberg::Vst::VSTGUIEditor (editController, &panelViewRect ())
{
}

//------------------------------------------------------------------------
Controller* Editor::pluginController ()
{
	return static_cast<Controller*> (getController ());
}

//------------------------------------------------------------------------
// void* parent: the native window handle, per the IPlugView contract.
bool PLUGIN_API Editor::open (void* parent, const PlatformType& platformType) // NOSONAR
{
	const CRect panelRect (0, 0, 420, 220);
	frame = new CFrame (panelRect, this); // NOSONAR
	frame->setBackgroundColor (panelColor);
	buildControls ();
	frame->open (parent, platformType);

	// ~30 ms: keep knobs/buttons/readouts in sync with automation playback.
	syncTimer = makeOwned<CVSTGUITimer> (
	    [this] (CVSTGUITimer*) {
		    syncFromParameters ();
	    },
	    30);
	return true;
}

//------------------------------------------------------------------------
void PLUGIN_API Editor::close ()
{
	syncTimer = nullptr;
	if (frame)
	{
		frame->close ();
		frame->forget ();
		frame = nullptr;
	}
}

//------------------------------------------------------------------------
void Editor::buildControls ()
{
	// --- Gain knob + readout ------------------------------------------
	addLabel (frame, CRect (20, 10, 110, 26), "Gain");
	auto* gainKnob = new VectorKnob (CRect (30, 32, 100, 102), this, kGainId); // NOSONAR
	gainKnob->setDefaultValue (static_cast<float> (gainDefaultNormalized ()));
	gainKnob->setValue (gainKnob->getDefaultValue ());
	frame->addView (gainKnob);
	gainReadout = addLabel (frame, CRect (10, 106, 120, 124), "0.0 dB");
	updateGainReadout (gainKnob);

	// --- Pan knob + readout ---------------------------------------------
	addLabel (frame, CRect (130, 10, 220, 26), "Pan");
	auto* panKnob = new VectorKnob (CRect (140, 32, 210, 102), this, kPanId); // NOSONAR
	panKnob->setDefaultValue (static_cast<float> (panDefaultNormalized ()));
	panKnob->setValue (panKnob->getDefaultValue ());
	frame->addView (panKnob);
	panReadout = addLabel (frame, CRect (130, 106, 220, 124), "C");
	updatePanReadout (panKnob);

	// --- Mono and Bypass buttons ----------------------------------------
	auto* monoButton = new CTextButton (CRect (240, 36, 320, 62), this, kMonoId, "Mono"); // NOSONAR
	monoButton->setStyle (CTextButton::kOnOffStyle);
	frame->addView (monoButton);

	auto* bypassButton =
	    new CTextButton (CRect (240, 74, 320, 100), this, kBypassId, "Bypass"); // NOSONAR
	bypassButton->setStyle (CTextButton::kOnOffStyle);
	frame->addView (bypassButton);

	// --- L/R meters with clip LEDs --------------------------------------
	frame->addView (new MeterView (CRect (342, 24, 358, 200), *pluginController (), 0)); // NOSONAR
	frame->addView (new MeterView (CRect (360, 24, 376, 200), *pluginController (), 1)); // NOSONAR

	syncFromParameters ();
}

//------------------------------------------------------------------------
void Editor::valueChanged (CControl* control)
{
	// Knob/button moves go through the host so they are recorded as
	// automation (plugin-ui spec). The begin/end edit brackets arrive via
	// controlBeginEdit/controlEndEdit.
	if (auto* editController = getController ();
	    editController != nullptr)
	{
		editController->performEdit (static_cast<Steinberg::Vst::ParamID> (control->getTag ()),
		                             control->getValue ());
	}

	if (control->getTag () == kGainId)
		updateGainReadout (control);
	else if (control->getTag () == kPanId)
		updatePanReadout (control);
}

//------------------------------------------------------------------------
void Editor::controlBeginEdit (CControl* control)
{
	if (auto* editController = getController (); editController != nullptr)
		editController->beginEdit (static_cast<Steinberg::Vst::ParamID> (control->getTag ()));
}

//------------------------------------------------------------------------
void Editor::controlEndEdit (CControl* control)
{
	if (auto* editController = getController (); editController != nullptr)
		editController->endEdit (static_cast<Steinberg::Vst::ParamID> (control->getTag ()));
}

//------------------------------------------------------------------------
void Editor::syncFromParameters ()
{
	auto* editController = getController ();
	if (!editController)
		return;

	auto update = [this, editController] (CView* view) {
		auto* control = dynamic_cast<CControl*> (view);
		if (!control || control->getTag () < 0 || control->isEditing ())
			return;
		const auto tag = static_cast<Steinberg::Vst::ParamID> (control->getTag ());
		const auto value = editController->getParamNormalized (tag);
		if (std::abs (static_cast<double> (control->getValue ()) - value) > 0.0005)
		{
			control->setValue (static_cast<float> (value));
			control->invalid ();
			if (tag == kGainId)
				updateGainReadout (control);
			else if (tag == kPanId)
				updatePanReadout (control);
		}
	};

	// Knobs and buttons (meters poll their own timer).
	frame->forEachChild ([&update] (CView* view) {
		const auto* control = dynamic_cast<const CControl*> (view);
		if (control && (control->getTag () == kGainId || control->getTag () == kPanId ||
		                control->getTag () == kMonoId || control->getTag () == kBypassId))
			update (view);
	});
}

//------------------------------------------------------------------------
void Editor::updateGainReadout (const CControl* knob)
{
	if (!gainReadout)
		return;
	const int index = gainNormalizedToIndex (knob->getValue ());
	std::array<char, 32> text {};
	if (index == 0)
		std::format_to_n (text.data (), text.size () - 1, "-\xE2\x88\x9E dB");
	else
		std::format_to_n (text.data (), text.size () - 1, "{:.1f} dB", gainIndexToDb (index));
	gainReadout->setText (text.data ());
}

//------------------------------------------------------------------------
void Editor::updatePanReadout (const CControl* knob)
{
	if (!panReadout)
		return;
	const float pan = panFromNormalized (knob->getValue ());
	const auto offset = static_cast<int> (std::lround (std::fabs (pan) * 100.f));
	std::array<char, 16> text {};
	if (offset == 0)
		std::format_to_n (text.data (), text.size () - 1, "C");
	else if (pan < 0.f)
		std::format_to_n (text.data (), text.size () - 1, "L{}", offset);
	else
		std::format_to_n (text.data (), text.size () - 1, "R{}", offset);
	panReadout->setText (text.data ());
}

} // namespace Radixhomework
