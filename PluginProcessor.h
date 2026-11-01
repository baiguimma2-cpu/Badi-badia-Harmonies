#pragma once
#include <JuceHeader.h>

// Badi-badia Harmony — génère des harmonies diatoniques à partir d'une seule voix.
// DSP : détection de hauteur (autocorrélation décimée) + transposeurs granulaires
// à deux têtes sur un tampon partagé. Très faible consommation CPU.
class HarmonyProcessor : public juce::AudioProcessor
{
public:
    static constexpr int numVoices = 6;

    HarmonyProcessor();
    ~HarmonyProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Badi-badia Harmony"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    static juce::String pitchClassName (int pc);
    static juce::String noteName (float midi);

    juce::AudioProcessorValueTreeState apvts;

    // Lus par l'éditeur (affichage)
    std::atomic<float> detectedMidi { -1.f };
    std::atomic<float> level { 0.f };
    std::atomic<float> voiceMidi[numVoices];

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void feedDetector (float x);
    void runDetect();
    void updateTargets();
    float readDelay (float d) const noexcept;

    std::atomic<float>* pKey = nullptr;
    std::atomic<float>* pMode = nullptr;
    std::atomic<float>* pDry = nullptr;
    std::atomic<float>* pWet = nullptr;
    std::atomic<float>* pWidth = nullptr;
    std::atomic<float>* pVoice[numVoices] {};

    double sampleRate = 44100.0;
    float Wf = 2205.f;
    int dryN = 1;

    std::vector<float> ring, dryL, dryR, anaBuf, tmp, corr;
    int ringMask = 0, writePos = 0, dryPos = 0;
    int decFactor = 2, decCount = 0, anaPos = 0, hop = 0, unvoiced = 0;
    float decAcc = 0.f, curMidi = 0.f;
    bool haveMidi = false;

    int keyV = 0;
    bool minorV = false;
    bool enabled[numVoices] {};

    struct Voice
    {
        float phase = 0.f;
        juce::SmoothedValue<float> ratio, gain;
    };
    Voice voices[numVoices];
    juce::SmoothedValue<float> dryS, wetS;
    float panL[numVoices] {}, panR[numVoices] {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HarmonyProcessor)
};
