#pragma once
#include "Logger.h"

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
GE::ERROR_CODE SafeShutdownReturnsErrorCode(T *&ptr) {
	ERROR_CODE result = ERROR_CODE::NULL_POINTER;
	if (ptr) {
		result = ptr->Shutdown();
		delete ptr;
		ptr = nullptr;
		return result;
	}
	GE_LOG_TRACE("Pointer is already null.");
	return result;
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