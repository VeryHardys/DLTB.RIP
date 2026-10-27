// Native StatChangeEffect values are IEEE-754 float32, not integer percentages.
// Calculate in double first; never dispatch an overflowed scalar or product.
static bool owned_encode_magnitude(double magnitude, float factor, float* raw) {
    if (!raw || !_finite(magnitude) || !_finite(factor) || factor == 0.0f ||
        magnitude < -(double)FLT_MAX || magnitude > (double)FLT_MAX) return false;
    const double quotient = magnitude / (double)factor;
    if (!_finite(quotient) || quotient < -(double)FLT_MAX || quotient > (double)FLT_MAX)
        return false;
    const float encoded = (float)quotient;
    const float displayed = encoded * factor;
    if (!_finite(encoded) || !_finite(displayed) ||
        (magnitude != 0.0 && (encoded == 0.0f || displayed == 0.0f))) return false;
    *raw = encoded;
    return true;
}
