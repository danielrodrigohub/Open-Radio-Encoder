// Biquad filter — from BUTT Biquad.h
#pragma once
namespace ore {
class Biquad {
public:
    enum Type { bq_type_lowpass = 0, bq_type_highpass, bq_type_bandpass, bq_type_notch, bq_type_peak, bq_type_lowshelf, bq_type_highshelf };
    Biquad(int type, double Fc, double Q, double peakGainDB);
    ~Biquad() = default;
    float process(float in);
    void setPeakGain(double peakGainDB);
    void setBiquad(int type, double Fc, double Q, double peakGainDB);
private:
    void calcBiquad();
    int type_ = bq_type_lowpass;
    double a0_, a1_, a2_, b1_, b2_;
    double Fc_, Q_, peakGain_;
    double z1_ = 0, z2_ = 0;
};
} // namespace ore
