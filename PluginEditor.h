#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class BBLookAndFeel : public juce::LookAndFeel_V4
{
public:
    BBLookAndFeel();
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&,
                               bool highlighted, bool down) override;
};

class HarmonyEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit HarmonyEditor (HarmonyProcessor&);
    ~HarmonyEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    using CA = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;

    HarmonyProcessor& proc;
    BBLookAndFeel laf;

    juce::ComboBox keyBox, modeBox;
    juce::ToggleButton voiceBtn[HarmonyProcessor::numVoices];
    juce::TextButton presetBtn[5];
    juce::Slider dryS, wetS, widthS;

    juce::String noteText { "—" };
    float lvl = 0.f, lastLvl = 0.f;
    const juce::Rectangle<int> noteArea { 20, 92, 520, 100 };

    std::unique_ptr<CA> keyAtt, modeAtt;
    std::unique_ptr<BA> voiceAtt[HarmonyProcessor::numVoices];
    std::unique_ptr<SA> dryAtt, wetAtt, widthAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HarmonyEditor)
};
