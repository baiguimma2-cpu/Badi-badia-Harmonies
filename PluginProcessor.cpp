#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    constexpr int kAna = 1024;
    constexpr int kInterval[HarmonyProcessor::numVoices] = { 0, 2, 4, 7, -2, -7 };
    constexpr float kPan[HarmonyProcessor::numVoices] = { -0.3f, 0.5f, -0.5f, 0.8f, -0.8f, 0.2f };
    constexpr int kMajor[7] = { 0, 2, 4, 5, 7, 9, 11 };
    constexpr int kMinor[7] = { 0, 2, 3, 5, 7, 8, 10 };
    const char* const kNames[12] = { "Do", "Do♯", "Ré", "Ré♯", "Mi", "Fa", "Fa♯", "Sol", "Sol♯", "La", "La♯", "Si" };

    int scaleNote (int D, int key, bool minor)
    {
        const int* s = minor ? kMinor : kMajor;
        const int oct = (int) std::floor ((double) D / 7.0);
        return oct * 12 + key + s[D - oct * 7];
    }

    // Note de la gamme la plus proche de m, puis décalage de d degrés.
    int harmonise (float m, int d, int key, bool minor)
    {
        const int D0 = (int) std::round ((m - (float) key) * 7.f / 12.f);
        int best = D0;
        float bd = 1e9f;
        for (int D = D0 - 4; D <= D0 + 4; ++D)
        {
            const float e = std::abs ((float) scaleNote (D, key, minor) - m);
            if (e < bd) { bd = e; best = D; }
        }
        return scaleNote (best + d, key, minor);
    }
}

juce::String HarmonyProcessor::pitchClassName (int pc)
{
    return juce::String::fromUTF8 (kNames[((pc % 12) + 12) % 12]);
}

juce::String HarmonyProcessor::noteName (float midi)
{
    const int r = juce::roundToInt (midi);
    return pitchClassName (r) + juce::String ((int) std::floor (r / 12.0) - 1);
}

juce::AudioProcessorValueTreeState::ParameterLayout HarmonyProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout L;

    juce::StringArray keys;
    for (int i = 0; i < 12; ++i) keys.add (pitchClassName (i));
    L.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "key", 1 }, "Tonalite", keys, 0));
    L.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "mode", 1 }, "Mode",
                                                         juce::StringArray { "Majeur", "Mineur" }, 0));

    for (int i = 0; i < numVoices; ++i)
        L.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "v" + juce::String (i), 1 },
                                                           "Voix " + juce::String (i + 1), i == 1 || i == 2));

    auto pct = juce::AudioParameterFloatAttributes().withStringFromValueFunction (
        [] (float v, int) { return juce::String (juce::roundToInt (v * 100.f)) + " %"; });
    L.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "dry", 1 }, "Ma voix",
                                                        juce::NormalisableRange<float> (0.f, 1.f), 1.f, pct));
    L.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "wet", 1 }, "Harmonies",
                                                        juce::NormalisableRange<float> (0.f, 1.f), 0.7f, pct));
    L.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "width", 1 }, "Largeur",
                                                        juce::NormalisableRange<float> (0.f, 1.f), 0.8f, pct));
    return L;
}

HarmonyProcessor::HarmonyProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    pKey = apvts.getRawParameterValue ("key");
    pMode = apvts.getRawParameterValue ("mode");
    pDry = apvts.getRawParameterValue ("dry");
    pWet = apvts.getRawParameterValue ("wet");
    pWidth = apvts.getRawParameterValue ("width");
    for (int i = 0; i < numVoices; ++i)
    {
        pVoice[i] = apvts.getRawParameterValue ("v" + juce::String (i));
        voiceMidi[i].store (0.f);
    }
}

bool HarmonyProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto& out = l.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo()) return false;
    return l.getMainInputChannelSet() == out;
}

void HarmonyProcessor::prepareToPlay (double sr, int)
{
    sampleRate = sr;
    Wf = (float) (0.05 * sr);                       // fenêtre de 50 ms
    const int W = (int) Wf;
    ring.assign ((size_t) juce::nextPowerOfTwo (W * 2 + 8), 0.f);
    ringMask = (int) ring.size() - 1;
    writePos = 0;

    dryN = juce::jmax (1, W / 2);                    // voix directe alignée sur les harmonies
    dryL.assign ((size_t) dryN, 0.f);
    dryR.assign ((size_t) dryN, 0.f);
    dryPos = 0;
    setLatencySamples (dryN);

    decFactor = juce::jmax (1, (int) std::round (sr / 22050.0));
    anaBuf.assign (kAna, 0.f);
    tmp.assign (kAna, 0.f);
    corr.assign (512, 0.f);
    anaPos = decCount = hop = unvoiced = 0;
    decAcc = 0.f;
    haveMidi = false;

    for (auto& v : voices)
    {
        v.phase = 0.f;
        v.ratio.reset (sr, 0.03);
        v.ratio.setCurrentAndTargetValue (1.f);
        v.gain.reset (sr, 0.03);
        v.gain.setCurrentAndTargetValue (0.f);
    }
    dryS.reset (sr, 0.02);
    wetS.reset (sr, 0.02);
    dryS.setCurrentAndTargetValue (pDry->load());
    wetS.setCurrentAndTargetValue (0.f);
}

inline float HarmonyProcessor::readDelay (float d) const noexcept
{
    const float rp = (float) writePos - d;
    const int i0 = (int) std::floor (rp);
    const float fr = rp - (float) i0;
    const float a = ring[(size_t) (i0 & ringMask)];
    const float b = ring[(size_t) ((i0 + 1) & ringMask)];
    return a + (b - a) * fr;
}

void HarmonyProcessor::updateTargets()
{
    if (! haveMidi) return;
    for (int v = 0; v < numVoices; ++v)
    {
        const int t = harmonise (curMidi, kInterval[v], keyV, minorV);
        const float semis = juce::jlimit (-14.f, 14.f, (float) t - curMidi);
        const float r = std::exp2 (semis / 12.f);
        if (enabled[v]) voices[v].ratio.setTargetValue (r);
        else voices[v].ratio.setCurrentAndTargetValue (r);
        voiceMidi[v].store ((float) t);
    }
}

void HarmonyProcessor::feedDetector (float x)
{
    decAcc += x;
    if (++decCount < decFactor) return;
    anaBuf[(size_t) anaPos] = decAcc / (float) decFactor;
    anaPos = (anaPos + 1) & (kAna - 1);
    decAcc = 0.f;
    decCount = 0;
    if (++hop >= 256) { hop = 0; runDetect(); }
}

void HarmonyProcessor::runDetect()
{
    float e = 0.f;
    for (int i = 0; i < kAna; ++i)
    {
        const float s = anaBuf[(size_t) ((anaPos + i) & (kAna - 1))];
        tmp[(size_t) i] = s;
        e += s * s;
    }
    if (std::sqrt (e / (float) kAna) < 0.01f)
    {
        if (++unvoiced > 10) detectedMidi.store (-1.f);
        return;
    }

    const float ar = (float) (sampleRate / decFactor);
    const int lo = juce::jmax (2, (int) (ar / 800.f));
    const int hi = juce::jmin ((int) (ar / 80.f), 400);
    const int N = kAna - hi;
    float mx = 0.f;
    for (int l = lo; l <= hi; ++l)
    {
        float s = 0.f;
        for (int i = 0; i < N; i += 2) s += tmp[(size_t) i] * tmp[(size_t) (i + l)];
        corr[(size_t) l] = s;
        if (s > mx) mx = s;
    }
    if (mx <= 0.f) return;

    for (int l = lo + 1; l < hi; ++l)
    {
        const float a = corr[(size_t) (l - 1)], b = corr[(size_t) l], g = corr[(size_t) (l + 1)];
        if (b > 0.88f * mx && b >= a && b >= g)
        {
            const float den = a - 2.f * b + g;
            const float p = den != 0.f ? 0.5f * (a - g) / den : 0.f;
            const float m = 69.f + 12.f * std::log2 (ar / ((float) l + p) / 440.f);
            unvoiced = 0;
            curMidi = (! haveMidi || std::abs (m - curMidi) > 1.f) ? m : curMidi + (m - curMidi) * 0.5f;
            haveMidi = true;
            detectedMidi.store (curMidi);
            updateTargets();
            return;
        }
    }
}

void HarmonyProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    const int nch = juce::jmin (2, buffer.getNumChannels());
    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, n);
    if (nch == 0 || ring.empty()) return;

    keyV = (int) pKey->load();
    minorV = pMode->load() > 0.5f;
    const float width = pWidth->load();

    int active = 0;
    for (int v = 0; v < numVoices; ++v)
    {
        enabled[v] = pVoice[v]->load() > 0.5f;
        if (enabled[v]) ++active;
        voices[v].gain.setTargetValue (enabled[v] ? 1.f : 0.f);
        const float ang = (kPan[v] * width + 1.f) * juce::MathConstants<float>::pi * 0.25f;
        panL[v] = std::cos (ang);
        panR[v] = std::sin (ang);
    }
    updateTargets();
    dryS.setTargetValue (pDry->load());
    wetS.setTargetValue (pWet->load() * 0.85f / std::sqrt ((float) juce::jmax (1, active)));

    float* l = buffer.getWritePointer (0);
    float* r = nch > 1 ? buffer.getWritePointer (1) : nullptr;

    for (int i = 0; i < n; ++i)
    {
        const float inL = l[i];
        const float inR = r != nullptr ? r[i] : inL;
        const float mono = r != nullptr ? 0.5f * (inL + inR) : inL;

        ring[(size_t) writePos] = mono;
        feedDetector (mono);

        const float dL = dryL[(size_t) dryPos], dR = dryR[(size_t) dryPos];
        dryL[(size_t) dryPos] = inL;
        dryR[(size_t) dryPos] = inR;
        if (++dryPos >= dryN) dryPos = 0;

        float hl = 0.f, hr = 0.f;
        for (int v = 0; v < numVoices; ++v)
        {
            auto& vc = voices[v];
            if (! vc.gain.isSmoothing() && vc.gain.getTargetValue() == 0.f) continue;

            const float g = vc.gain.getNextValue();
            const float ratio = vc.ratio.getNextValue();
            vc.phase += (1.f - ratio) / Wf;
            vc.phase -= std::floor (vc.phase);
            float p2 = vc.phase + 0.5f;
            if (p2 >= 1.f) p2 -= 1.f;
            const float s = std::sin (juce::MathConstants<float>::pi * vc.phase);
            const float w1 = s * s;
            const float y = (readDelay (1.f + vc.phase * Wf) * w1
                             + readDelay (1.f + p2 * Wf) * (1.f - w1)) * g;
            hl += y * panL[v];
            hr += y * panR[v];
        }

        const float dg = dryS.getNextValue(), wg = wetS.getNextValue();
        if (r != nullptr) { l[i] = dL * dg + hl * wg; r[i] = dR * dg + hr * wg; }
        else              { l[i] = dL * dg + 0.5f * (hl + hr) * wg; }

        writePos = (writePos + 1) & ringMask;
    }

    level.store (juce::jmin (1.f, buffer.getMagnitude (0, n)));
}

juce::AudioProcessorEditor* HarmonyProcessor::createEditor() { return new HarmonyEditor (*this); }

void HarmonyProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, dest);
}

void HarmonyProcessor::setStateInformation (const void* data, int size)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, size));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new HarmonyProcessor(); }
