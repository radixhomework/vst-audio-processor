//------------------------------------------------------------------------
// Edit controller — parameter registration, the editor (tasks 5.x) and
// the receiving end of the meter data exchange (design D5/D6): blocks
// from the processor update a lock-free cache the MeterView polls.
//------------------------------------------------------------------------
#pragma once

#include "dsp/meter_state.h"

#include "pluginterfaces/vst/ivstdataexchange.h"
#include "public.sdk/source/vst/utility/dataexchange.h"
#include "public.sdk/source/vst/vsteditcontroller.h"

namespace Radixhomework {

class Controller : public Steinberg::Vst::EditController,
                   public Steinberg::Vst::IDataExchangeReceiver
{
public:
	static Steinberg::FUnknown* createInstance (void* /*context*/);

	//---Interface---------------------------------------------------------
	OBJ_METHODS (Controller, Steinberg::Vst::EditController)
	DEFINE_INTERFACES
		DEF_INTERFACE (Steinberg::Vst::IDataExchangeReceiver)
	END_DEFINE_INTERFACES (Steinberg::Vst::EditController)
	REFCOUNT_METHODS (Steinberg::Vst::EditController)

	Steinberg::tresult PLUGIN_API initialize (Steinberg::FUnknown* context) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API setComponentState (Steinberg::IBStream* state) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API notify (Steinberg::Vst::IMessage* message) SMTG_OVERRIDE;

	//---from IEditController----------------------------------------------
	Steinberg::IPlugView* PLUGIN_API createView (Steinberg::FIDString name) SMTG_OVERRIDE;

	//---from IDataExchangeReceiver----------------------------------------
	void PLUGIN_API queueOpened (Steinberg::Vst::DataExchangeUserContextID userContextID,
	                             Steinberg::uint32 blockSize,
	                             Steinberg::TBool& dispatchOnBackgroundThread) SMTG_OVERRIDE;
	void PLUGIN_API queueClosed (Steinberg::Vst::DataExchangeUserContextID userContextID) SMTG_OVERRIDE;
	void PLUGIN_API onDataExchangeBlocksReceived (
	    Steinberg::Vst::DataExchangeUserContextID userContextID, Steinberg::uint32 numBlocks,
	    Steinberg::Vst::DataExchangeBlock* block, Steinberg::TBool onBackgroundThread) SMTG_OVERRIDE;

	// Editor access (tasks 5.x).
	MeterState& meterCache () { return meterStateCache; }

	/// Clears the cached clip flag and asks the processor to clear its
	/// latched flag (completes the 4.2 reset path).
	void requestClipReset (int channel);

private:
	Steinberg::Vst::DataExchangeReceiverHandler meterExchange {this};
	MeterState meterStateCache;
};

} // namespace Radixhomework
