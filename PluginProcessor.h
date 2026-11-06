#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <algorithm>
#include <atomic>
#include <vector>

//==============================================================================
// Parameter IDs (one place, so the processor and editor always agree).
//==============================================================================
namespace ParamIDs
{
    inline constexpr const char* instGain   = "instGain";    // instrument channel gain
    inline constexpr const char* vocalGain  = "vocalGain";   // vocal channel gain
    inline constexpr const char* masterGain = "masterGain";  // master out gain
}

//==============================================================================
// VocalMixerAudioProcessor
// Stage 2: bus layout, per-channel gains, sum to a master output, and a
// constant latency report. (Ducking arrives in stage 5.)
//
// Buses:
//   Input  0 (main)  : stereo  - instruments
//   Input  1 (aux)   : stereo  - vocal
//   Output 0 (main)  : stereo  - mix
//==============================================================================
class VocalMixerAudioProcessor : public juce::AudioProcessor
{
public:
    VocalMixerAudioProcessor();
    ~VocalMixerAudioProcessor() override = default;

    //--------------------------------------------------------------------------
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //--------------------------------------------------------------------------
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //--------------------------------------------------------------------------
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    //--------------------------------------------------------------------------
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    //--------------------------------------------------------------------------
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //--------------------------------------------------------------------------
    // Constant latency (samples) reported to the host. This matches the
    // 512-sample STFT ducker added in stage 5, so both paths are delayed by
    // the same amount and stay time-aligned.
    static constexpr int kLatencySamples = 512;

    // Number of channels per bus (everything is stereo).
    static constexpr int kNumChannels = 2;

    // All plugin parameters live here.
    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    //--------------------------------------------------------------------------
    // Minimal fixed-length delay line (allocates only in prepare()).
    // Used in stage 2 as a stand-in for the STFT latency so the reported
    // latency is genuine. Stage 5 replaces the instrument delay with the ducker.
    //--------------------------------------------------------------------------
    class SimpleDelay
    {
    public:
        void prepare (int numChannels, int delaySamples)
        {
            length = juce::jmax (1, delaySamples);
            lines.assign (static_cast<size_t> (numChannels),
                          std::vector<float> (static_cast<size_t> (length), 0.0f));
            writePos.assign (static_cast<size_t> (numChannels), 0);
        }

        void reset()
        {
            for (auto& line : lines)
                std::fill (line.begin(), line.end(), 0.0f);
            std::fill (writePos.begin(), writePos.end(), 0);
        }

        // Push one sample in, get the sample from exactly 'length' samples ago.
        float process (int channel, float input) noexcept
        {
            auto& line = lines[static_cast<size_t> (channel)];
            int& pos   = writePos[static_cast<size_t> (channel)];

            const float output = line[static_cast<size_t> (pos)];
            line[static_cast<size_t> (pos)] = input;

            if (++pos >= length)
                pos = 0;

            return output;
        }

    private:
        int length = 1;
        std::vector<std::vector<float>> lines;
        std::vector<int> writePos;
    };

    //--------------------------------------------------------------------------
    // Cached pointers to the raw (atomic) parameter values - safe to read on
    // the audio thread.
    std::atomic<float>* instGainParam   = nullptr;
    std::atomic<float>* vocalGainParam  = nullptr;
    std::atomic<float>* masterGainParam = nullptr;

    // Smoothed linear gains, to avoid zipper noise when knobs move.
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> instGainSmooth;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> vocalGainSmooth;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> masterGainSmooth;

    // One delay per path so instruments and vocal stay aligned.
    SimpleDelay instDelay;
    SimpleDelay vocalDelay;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VocalMixerAudioProcessor)
};
