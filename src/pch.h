#pragma once

// Windows
#include <Windows.h>
#include <Uxtheme.h>
#define STRSAFE_NO_CB_FUNCTIONS
#include <strsafe.h>
#include <xmmintrin.h>
#include <shellapi.h>
#define STRICT_TYPED_ITEMIDS
#include <Shlobj.h>
#include <d2d1_1.h> // Needed?
#include <d2d1.h>
#include <dwrite.h>
#include <wincodec.h> // WIC
#include <dwmapi.h>
#include <d3d11.h>
#include <dcomp.h>
#include <vssym32.h>

// Standard library
#include <new>
#include <string.h>
#include <stdint.h>

#include "../3rdparty/tracy/Tracy.hpp"
#include "../3rdparty/tracy/common/TracySystem.hpp"
