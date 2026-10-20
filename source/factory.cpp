//------------------------------------------------------------------------
// Plug-in factory / module entry: registers the processor and controller
// classes with identity from version.h.
//------------------------------------------------------------------------

#include "controller.h"
#include "processor.h"
#include "version.h"

#include "public.sdk/source/main/pluginfactory.h"

BEGIN_FACTORY (stringCompanyName, stringCompanyWeb, stringCompanyEmail,
               Steinberg::PFactoryInfo::kUnicode)

	//--- The audio effect component --------------------------------------
	DEF_CLASS2 (INLINE_UID_FROM_FUID (audioProcessorProcessorUID),
	            Steinberg::PClassInfo::kManyInstances,
	            kVstAudioEffectClass,
	            stringPluginName,
	            Steinberg::Vst::kDistributable,
	            audioProcessorVST3Category,
	            FULL_VERSION_STR,
	            kVstVersionString,
	            Radixhomework::Processor::createInstance)

	//--- Its edit controller ----------------------------------------------
	DEF_CLASS2 (INLINE_UID_FROM_FUID (audioProcessorControllerUID),
	            Steinberg::PClassInfo::kManyInstances,
	            kVstComponentControllerClass,
	            stringPluginName " Controller",
	            0,
	            "",
	            FULL_VERSION_STR,
	            kVstVersionString,
	            Radixhomework::Controller::createInstance)

END_FACTORY
