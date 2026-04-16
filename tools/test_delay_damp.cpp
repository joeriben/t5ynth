// Standalone test: verify delay damp filter actually affects output
// Compile: c++ -std=c++17 -I../JUCE/modules -DJUCE_MODULE_AVAILABLE_juce_dsp=1 -DJUCE_MODULE_AVAILABLE_juce_audio_basics=1 -framework Accelerate tools/test_delay_damp.cpp -o tools/test_delay_damp
//
// Simpler approach: just test the IIR math directly.

#include <cmath>
#include <cstdio>
#include <cstring>

// Minimal biquad LP filter (transposed direct form II)
struct BiquadLP {
    float b0, b1, b2, a1, a2;
    float s0 = 0, s1 = 0;

    void setLowPass(double sr, float freq, float Q = 0.707f) {
        float n = 1.0f / std::tan(M_PI * freq / sr);
        float nSq = n * n;
        float invQ = 1.0f / Q;
        float c1 = 1.0f / (1.0f + invQ * n + nSq);
        b0 = c1;
        b1 = c1 * 2;
        b2 = c1;
        a1 = c1 * 2 * (1 - nSq);
        a2 = c1 * (1 - invQ * n + nSq);
    }

    float process(float x) {
        float y = b0 * x + s0;
        s0 = b1 * x - a1 * y + s1;
        s1 = b2 * x - a2 * y;
        return y;
    }

    void reset() { s0 = s1 = 0; }
};

int main() {
    const double sr = 44100.0;
    const int delaySamples = (int)(250.0 * 0.001 * sr); // 250ms delay
    const float feedback = 0.7f;
    const float wetMix = 0.5f;
    const float dryGain = 1.0f - wetMix * 0.3f;

    // Simple delay line
    const int maxDelay = 44100 * 5;
    float delayBuf[maxDelay];
    int writePos = 0;

    auto popSample = [&]() -> float {
        int readPos = writePos - delaySamples;
        if (readPos < 0) readPos += maxDelay;
        return delayBuf[readPos];
    };
    auto pushSample = [&](float v) {
        delayBuf[writePos] = v;
        writePos = (writePos + 1) % maxDelay;
    };

    // Test with two damp settings
    float dampValues[] = { 0.0f, 0.5f, 1.0f };

    for (float damp : dampValues) {
        // Reset delay line and filter
        memset(delayBuf, 0, sizeof(delayBuf));
        writePos = 0;

        BiquadLP filter;
        float dampFreq = 20000.0f * std::pow(500.0f / 20000.0f, damp);
        filter.setLowPass(sr, dampFreq);

        // Generate test signal: 1000 Hz sine for 100 samples, then silence
        const int totalSamples = delaySamples * 6; // enough for several echoes
        float rmsEcho[5] = {};
        int echoCount = 0;

        for (int i = 0; i < totalSamples; ++i) {
            float input = (i < 100) ? std::sin(2.0 * M_PI * 1000.0 * i / sr) : 0.0f;

            float delayed = popSample();
            float fbSample = delayed * feedback;
            float dampedFb = filter.process(fbSample);
            pushSample(input + dampedFb);

            float output = input * dryGain + delayed * wetMix;

            // Measure echo RMS in windows around expected echo times
            for (int e = 0; e < 5; ++e) {
                int echoStart = delaySamples * (e + 1) - 50;
                int echoEnd = delaySamples * (e + 1) + 50;
                if (i >= echoStart && i < echoEnd) {
                    rmsEcho[e] += output * output;
                }
            }
        }

        printf("damp=%.1f (LP cutoff=%.0f Hz):\n", damp, dampFreq);
        for (int e = 0; e < 5; ++e) {
            rmsEcho[e] = std::sqrt(rmsEcho[e] / 100.0f);
            printf("  echo %d: RMS=%.6f\n", e + 1, rmsEcho[e]);
        }
        printf("\n");
    }

    // Also verify that the mapping formula produces expected results
    printf("Damp parameter mapping:\n");
    for (float d = 0; d <= 1.01f; d += 0.25f) {
        float freq = 20000.0f * std::pow(500.0f / 20000.0f, d);
        printf("  d=%.2f -> cutoff=%.0f Hz\n", d, freq);
    }

    printf("\nDisplay format mapping (fmtDampHz):\n");
    for (float v = 0; v <= 1.01f; v += 0.25f) {
        double hz = 200.0 * std::pow(100.0, v);
        printf("  v=%.2f -> display=%.0f Hz\n", v, hz);
    }

    return 0;
}
