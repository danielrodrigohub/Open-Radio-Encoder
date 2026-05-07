#include "biquad.h"
#include <cmath>

namespace ore {

Biquad::Biquad(int type, double Fc, double Q, double peakGainDB, double sampleRate)
    : type_(type), Fc_(Fc), Q_(Q), peakGain_(peakGainDB), sampleRate_(sampleRate),
      a0_(1), a1_(0), a2_(0), b1_(0), b2_(0) { calcBiquad(); }

float Biquad::process(float in) {
    double out = in * a0_ + z1_;
    z1_ = in * a1_ + z2_ - b1_ * out;
    z2_ = in * a2_ - b2_ * out;
    return static_cast<float>(out);
}

void Biquad::setPeakGain(double g) { peakGain_ = g; calcBiquad(); }
void Biquad::setBiquad(int t, double f, double q, double g) {
    type_ = t; Fc_ = f; Q_ = q; peakGain_ = g; calcBiquad();
}

void Biquad::calcBiquad() {
    if (Fc_ <= 0.0 || sampleRate_ <= 0.0) {
        a0_ = 1; a1_ = 0; a2_ = 0; b1_ = 0; b2_ = 0;
        return;
    }

    double w0 = 2.0 * M_PI * Fc_ / sampleRate_;
    double sin_w0 = std::sin(w0);
    double cos_w0 = std::cos(w0);
    double alpha = sin_w0 / (2.0 * Q_);
    double A = std::pow(10.0, peakGain_ / 40.0);

    double b0 = 0, b1 = 0, b2 = 0, a0 = 0, a1 = 0, a2 = 0;

    switch (type_) {
    case bq_type_lowpass:
        b0 = (1.0 - cos_w0) / 2.0;
        b1 = 1.0 - cos_w0;
        b2 = (1.0 - cos_w0) / 2.0;
        a0 = 1.0 + alpha;
        a1 = -2.0 * cos_w0;
        a2 = 1.0 - alpha;
        break;

    case bq_type_highpass:
        b0 = (1.0 + cos_w0) / 2.0;
        b1 = -(1.0 + cos_w0);
        b2 = (1.0 + cos_w0) / 2.0;
        a0 = 1.0 + alpha;
        a1 = -2.0 * cos_w0;
        a2 = 1.0 - alpha;
        break;

    case bq_type_bandpass:
        b0 = sin_w0 / 2.0;
        b1 = 0.0;
        b2 = -sin_w0 / 2.0;
        a0 = 1.0 + alpha;
        a1 = -2.0 * cos_w0;
        a2 = 1.0 - alpha;
        break;

    case bq_type_notch:
        b0 = 1.0;
        b1 = -2.0 * cos_w0;
        b2 = 1.0;
        a0 = 1.0 + alpha;
        a1 = -2.0 * cos_w0;
        a2 = 1.0 - alpha;
        break;

    case bq_type_peak: {
        double alpha_peak = sin_w0 * std::sinh(std::log(2.0) / 2.0 * Q_ * w0 / sin_w0);
        if (Q_ <= 0.0) alpha_peak = sin_w0 / 2.0;
        b0 = 1.0 + alpha_peak * A;
        b1 = -2.0 * cos_w0;
        b2 = 1.0 - alpha_peak * A;
        a0 = 1.0 + alpha_peak / A;
        a1 = -2.0 * cos_w0;
        a2 = 1.0 - alpha_peak / A;
        break;
    }

    case bq_type_lowshelf: {
        double sqrtA = std::sqrt(A);
        double alpha_shelf = sin_w0 / 2.0 * std::sqrt((A + 1.0 / A) * (1.0 / Q_ - 1.0) + 2.0);
        b0 =     A * ((A + 1.0) - (A - 1.0) * cos_w0 + 2.0 * sqrtA * alpha_shelf);
        b1 = 2.0 * A * ((A - 1.0) - (A + 1.0) * cos_w0);
        b2 =     A * ((A + 1.0) - (A - 1.0) * cos_w0 - 2.0 * sqrtA * alpha_shelf);
        a0 =          (A + 1.0) + (A - 1.0) * cos_w0 + 2.0 * sqrtA * alpha_shelf;
        a1 = -2.0 *  ((A - 1.0) + (A + 1.0) * cos_w0);
        a2 =          (A + 1.0) + (A - 1.0) * cos_w0 - 2.0 * sqrtA * alpha_shelf;
        break;
    }

    case bq_type_highshelf: {
        double sqrtA = std::sqrt(A);
        double alpha_shelf = sin_w0 / 2.0 * std::sqrt((A + 1.0 / A) * (1.0 / Q_ - 1.0) + 2.0);
        b0 =     A * ((A + 1.0) + (A - 1.0) * cos_w0 + 2.0 * sqrtA * alpha_shelf);
        b1 = -2.0*A * ((A - 1.0) + (A + 1.0) * cos_w0);
        b2 =     A * ((A + 1.0) + (A - 1.0) * cos_w0 - 2.0 * sqrtA * alpha_shelf);
        a0 =          (A + 1.0) - (A - 1.0) * cos_w0 + 2.0 * sqrtA * alpha_shelf;
        a1 =  2.0 *  ((A - 1.0) - (A + 1.0) * cos_w0);
        a2 =          (A + 1.0) - (A - 1.0) * cos_w0 - 2.0 * sqrtA * alpha_shelf;
        break;
    }
    }

    double norm = 1.0 / a0;
    a0_ = b0 * norm;
    a1_ = b1 * norm;
    a2_ = b2 * norm;
    b1_ = a1 * norm;
    b2_ = a2 * norm;
}

} // namespace ore
