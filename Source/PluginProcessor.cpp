#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    float dbToGain (float db) noexcept { return std::pow (10.0f, db / 20.0f); }
}

WaveBombzPUNCHAudioProcessor::WaveBombzPUNCHAudioProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                      .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "PUNCH_STATE", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout WaveBombzPUNCHAudioProcessor::createParameterLayout()
{
    using Float = juce::AudioParameterFloat;
    juce::AudioProcessorValueTreeState::ParameterLayout p;
    p.add (std::make_unique<Float> (juce::ParameterID { "attack", 1 }, "Attack", juce::NormalisableRange<float> { -100, 100, 0.1f }, 0));
    p.add (std::make_unique<Float> (juce::ParameterID { "sustain", 1 }, "Sustain", juce::NormalisableRange<float> { -100, 100, 0.1f }, 0));
    p.add (std::make_unique<Float> (juce::ParameterID { "punch", 1 }, "Punch", juce::NormalisableRange<float> { 0, 100, 0.1f }, 0));
    p.add (std::make_unique<Float> (juce::ParameterID { "mix", 1 }, "Mix", juce::NormalisableRange<float> { 0, 100, 0.1f }, 100));
    p.add (std::make_unique<Float> (juce::ParameterID { "input", 1 }, "Input", juce::NormalisableRange<float> { -24, 24, 0.01f }, 0));
    p.add (std::make_unique<Float> (juce::ParameterID { "output", 1 }, "Output", juce::NormalisableRange<float> { -24, 24, 0.01f }, 0));
    p.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "softClip", 1 }, "Soft Clip", true));
    p.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "oversampling", 1 }, "Oversampling", juce::StringArray { "Off", "2x", "4x", "8x" }, 1));
    return p;
}

void WaveBombzPUNCHAudioProcessor::prepareToPlay (double sr, int maxBlock)
{
    currentSampleRate = sr;
    const auto channels = getTotalNumOutputChannels();
    const auto safeBlockSize = juce::jmax (1, maxBlock);
    dryBuffer.setSize (channels, safeBlockSize, false, true, true);

    inputGain.reset (sr, 0.02);
    outputGain.reset (sr, 0.02);
    attackAmount.reset (sr, 0.015);
    sustainAmount.reset (sr, 0.015);
    punchAmount.reset (sr, 0.015);
    wetMix.reset (sr, 0.015);
    inputGain.setCurrentAndTargetValue (1.0f);
    outputGain.setCurrentAndTargetValue (1.0f);
    attackAmount.setCurrentAndTargetValue (0.0f);
    sustainAmount.setCurrentAndTargetValue (0.0f);
    punchAmount.setCurrentAndTargetValue (0.0f);
    wetMix.setCurrentAndTargetValue (1.0f);

    for (size_t i = 0; i < oversamplers.size(); ++i)
    {
        const auto stages = i + 1;
        oversamplers[i] = std::make_unique<juce::dsp::Oversampling<float>> (
            static_cast<size_t> (channels), stages,
            juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true);
        oversamplers[i]->initProcessing (static_cast<size_t> (safeBlockSize));
        oversamplers[i]->reset();
    }

    fastEnvelope = slowEnvelope = 0.0f;
    inputPeak.store (0.0f);
    outputPeak.store (0.0f);
    inputRms.store (0.0f);
    outputRms.store (0.0f);
}

void WaveBombzPUNCHAudioProcessor::releaseResources()
{
    for (auto& oversampler : oversamplers)
        oversampler.reset();
}

bool WaveBombzPUNCHAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto input = layouts.getMainInputChannelSet();
    const auto output = layouts.getMainOutputChannelSet();
    return input == output && (output == juce::AudioChannelSet::mono() || output == juce::AudioChannelSet::stereo());
}

void WaveBombzPUNCHAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const auto channels = buffer.getNumChannels();
    const auto totalSamples = buffer.getNumSamples();
    if (channels == 0 || totalSamples == 0 || dryBuffer.getNumSamples() == 0)
        return;

    inputGain.setTargetValue (dbToGain (parameters.getRawParameterValue ("input")->load()));
    outputGain.setTargetValue (dbToGain (parameters.getRawParameterValue ("output")->load()));
    attackAmount.setTargetValue (parameters.getRawParameterValue ("attack")->load() * 0.01f);
    sustainAmount.setTargetValue (parameters.getRawParameterValue ("sustain")->load() * 0.01f);
    punchAmount.setTargetValue (parameters.getRawParameterValue ("punch")->load() * 0.01f);
    wetMix.setTargetValue (parameters.getRawParameterValue ("mix")->load() * 0.01f);

    const bool clip = parameters.getRawParameterValue ("softClip")->load() > 0.5f;
    const int oversamplingMode = juce::jlimit (0, 3, static_cast<int> (parameters.getRawParameterValue ("oversampling")->load() + 0.5f));
    const float fastCoefficient = std::exp (-1.0f / static_cast<float> (currentSampleRate * 0.003));
    const float slowCoefficient = std::exp (-1.0f / static_cast<float> (currentSampleRate * 0.080));

    float peakIn = 0.0f, peakOut = 0.0f;
    double sumSquaresIn = 0.0, sumSquaresOut = 0.0;
    const int chunkCapacity = dryBuffer.getNumSamples();

    // Hosts may occasionally deliver a block larger than the prepareToPlay hint.
    // Process it in bounded chunks; no scratch-buffer allocation occurs on the audio thread.
    for (int offset = 0; offset < totalSamples; offset += chunkCapacity)
    {
        const int count = juce::jmin (chunkCapacity, totalSamples - offset);
        for (int channel = 0; channel < channels; ++channel)
            dryBuffer.copyFrom (channel, 0, buffer, channel, offset, count);

        for (int sample = 0; sample < count; ++sample)
        {
            const float inputLevel = inputGain.getNextValue();
            float detector = 0.0f;
            for (int channel = 0; channel < channels; ++channel)
            {
                auto* data = buffer.getWritePointer (channel, offset);
                data[sample] *= inputLevel;
                const float magnitude = std::abs (data[sample]);
                detector += magnitude;
                peakIn = juce::jmax (peakIn, magnitude);
                sumSquaresIn += static_cast<double> (data[sample]) * data[sample];
            }

            detector /= static_cast<float> (channels);
            fastEnvelope = fastCoefficient * fastEnvelope + (1.0f - fastCoefficient) * detector;
            slowEnvelope = slowCoefficient * slowEnvelope + (1.0f - slowCoefficient) * detector;
            const float transient = fastEnvelope - slowEnvelope;
            const float attack = attackAmount.getNextValue();
            const float sustain = sustainAmount.getNextValue();
            const float punch = punchAmount.getNextValue();
            const float shape = juce::jlimit (0.25f, 2.5f, 1.0f + attack * transient * 8.0f)
                              * juce::jlimit (0.25f, 2.0f, 1.0f + sustain * (slowEnvelope - 0.15f))
                              * (1.0f + punch * juce::jmax (0.0f, transient) * 5.0f);
            for (int channel = 0; channel < channels; ++channel)
                buffer.getWritePointer (channel, offset)[sample] *= shape;
        }

        if (clip)
        {
            if (oversamplingMode == 0)
            {
                for (int channel = 0; channel < channels; ++channel)
                    for (int sample = 0; sample < count; ++sample)
                    {
                        auto* data = buffer.getWritePointer (channel, offset);
                        data[sample] = std::tanh (data[sample]);
                    }
            }
            else if (auto* oversampler = oversamplers[static_cast<size_t> (oversamplingMode - 1)].get())
            {
                juce::dsp::AudioBlock<float> wholeBlock (buffer);
                auto chunk = wholeBlock.getSubBlock (static_cast<size_t> (offset), static_cast<size_t> (count));
                auto highRate = oversampler->processSamplesUp (chunk);
                for (size_t channel = 0; channel < highRate.getNumChannels(); ++channel)
                    for (size_t sample = 0; sample < highRate.getNumSamples(); ++sample)
                        highRate.getChannelPointer (channel)[sample] = std::tanh (highRate.getChannelPointer (channel)[sample]);
                oversampler->processSamplesDown (chunk);
            }
        }

        for (int sample = 0; sample < count; ++sample)
        {
            const float outputLevel = outputGain.getNextValue();
            const float mix = wetMix.getNextValue();
            for (int channel = 0; channel < channels; ++channel)
            {
                auto* data = buffer.getWritePointer (channel, offset);
                const float dry = dryBuffer.getReadPointer (channel)[sample];
                data[sample] = (dry * (1.0f - mix) + data[sample] * mix) * outputLevel;
                const float magnitude = std::abs (data[sample]);
                peakOut = juce::jmax (peakOut, magnitude);
                sumSquaresOut += static_cast<double> (data[sample]) * data[sample];
            }
        }
    }

    const double sampleCount = static_cast<double> (totalSamples) * static_cast<double> (channels);
    inputPeak.store (peakIn);
    outputPeak.store (peakOut);
    inputRms.store (static_cast<float> (std::sqrt (sumSquaresIn / sampleCount)));
    outputRms.store (static_cast<float> (std::sqrt (sumSquaresOut / sampleCount)));
}

juce::AudioProcessorEditor* WaveBombzPUNCHAudioProcessor::createEditor()
{
    return new WaveBombzPUNCHAudioProcessorEditor (*this);
}

void WaveBombzPUNCHAudioProcessor::getStateInformation (juce::MemoryBlock& destination)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destination);
}

void WaveBombzPUNCHAudioProcessor::setStateInformation (const void* data, int size)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, size));
    if (xml != nullptr && xml->hasTagName (parameters.state.getType()))
        parameters.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new WaveBombzPUNCHAudioProcessor();
}
