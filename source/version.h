//------------------------------------------------------------------------
// Plugin identity — single source of truth (design D3).
//
// The class IDs below were randomly generated once and are committed. They
// must never change after a release: hosts store them inside projects and
// would treat a changed ID as a different plug-in.
//------------------------------------------------------------------------
#pragma once

#include "pluginterfaces/base/funknown.h"

// Vendor identity (plugin-core spec: name "Audio Processor", vendor
// "Radixhomework"; "VST" must not appear in the product name).
inline constexpr const char* stringCompanyName = "Radixhomework";
inline constexpr const char* stringCompanyWeb = "";
inline constexpr const char* stringCompanyEmail = "";
inline constexpr const char* stringPluginName = "Audio Processor";
inline constexpr const char* stringPluginControllerName = "Audio Processor Controller";

// Version 0.1.0 — Round 1.
inline constexpr int MAJOR_VERSION_INT = 0;
inline constexpr int SUB_VERSION_INT = 1;
inline constexpr int RELEASE_VERSION_INT = 0;
inline constexpr int BUILD_VERSION_INT = 0;
inline constexpr const char* FULL_VERSION_STR = "0.1.0";

// Subcategory shown in host browsers: a utility effect.
inline constexpr const char* audioProcessorVST3Category = "Fx";

//------------------------------------------------------------------------
// Class IDs — never change these once released (see file header).
//------------------------------------------------------------------------
static const Steinberg::FUID audioProcessorProcessorUID (0xD8AC551F, 0x099F4CC7,
                                                         0x91D86D55, 0xF243BFF4);
static const Steinberg::FUID audioProcessorControllerUID (0x9441F014, 0x7DEB4E77,
                                                          0x91E70ACB, 0x7372868B);
