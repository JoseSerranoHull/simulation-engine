#pragma once
#include "core/Common.h"

namespace GE::Utilities {
#pragma region SafeDeleters
template <typename T>
void SafeShutdown(T *&ptr) {
	if (ptr) {
		ptr->Shutdown();
		delete ptr;
		ptr = nullptr;
		return;
	}
	GE_LOG_TRACE("Pointer is already null.");
}

template <typename T>
void SafeDelete(T *&ptr) {
	if (ptr) {
		delete ptr;
		ptr = nullptr;
		return;
	}
	GE_LOG_TRACE("Pointer is already null.");
}

template <typename T>
void SafeDeleteArray(T *&ptr) {
	if (ptr) {
		delete[] ptr;
		ptr = nullptr;
		return;
	}
	GE_LOG_TRACE("Pointer is already null.");
}
#pragma endregion
}
