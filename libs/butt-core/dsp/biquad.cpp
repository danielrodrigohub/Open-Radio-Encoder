// Biquad filter — stub implementation
#include "biquad.h"
#include <cmath>
namespace ore {
Biquad::Biquad(int type, double Fc, double Q, double peakGainDB)
    : type_(type), Fc_(Fc), Q_(Q), peakGain_(peakGainDB),
      a0_(1), a1_(0), a2_(0), b1_(0), b2_(0) { calcBiquad(); }
float Biquad::process(float in) {
    double out = in * a0_ + z1_;
    z1_ = in * a1_ + z2_ - b1_ * out;
    z2_ = in * a2_ - b2_ * out;
    return static_cast<float>(out);
}
void Biquad::setPeakGain(double g) { peakGain_ = g; calcBiquad(); }
void Biquad::setBiquad(int t, double f, double q, double g) { type_=t; Fc_=f; Q_=q; peakGain_=g; calcBiquad(); }
void Biquad::calcBiquad() {
    double norm;
    double V = pow(10, fabs(peakGain_) / 20.0);
    double K = tan(M_PI * Fc_);
    if (type_ == bq_type_peak) {
        if (peakGain_ >= 0) {
            norm = 1.0 / (1.0 + 1.0/Q_ * K + K * K);
            a0_ = (1.0 + V/Q_ * K + K * K) * norm;
            a1_ = 2.0 * (K * K - 1.0) * norm;
            a2_ = (1.0 - V/Q_ * K + K * K) * norm;
            b1_ = a1_;
            b2_ = (1.0 - 1.0/Q_ * K + K * K) * norm;
        } else {
            norm = 1.0 / (1.0 + V/Q_ * K + K * K);
            a0_ = (1.0 + 1.0/Q_ * K + K * K) * norm;
            a1_ = 2.0 * (K * K - 1.0) * norm;
            a2_ = (1.0 - 1.0/Q_ * K + K * K) * norm;
            b1_ = a1_;
            b2_ = (1.0 - V/Q_ * K + K * K) * norm;
        }
    } else { a0_ = 1; a1_ = 0; a2_ = 0; b1_ = 0; b2_ = 0; }
}
} // namespace ore
