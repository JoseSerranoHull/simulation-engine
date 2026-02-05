#include "Light.h"

/**
 * @brief Virtual destructor: Ensures proper cleanup of derived light types.
 * Defined as default in the implementation to satisfy compiler requirements
 * for polymorphic base classes.
 */
Light::~Light() = default;