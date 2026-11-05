#pragma once
#include <JuceHeader.h>
#include <array>
#include <memory>
class WaveBombzPUNCHAudioProcessor final : public juce::AudioProcessor {
public:
 WaveBombzPUNCHAudioProcessor(); ~WaveBombzPUNCHAudioProcessor() override = default;
 void prepareToPlay(double, int) override; void releaseResources() override;
 bool isBusesLayoutSupported(const BusesLayout&) const override;
 void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
 void processBlockBypassed(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
 juce::AudioProcessorEditor* createEditor() override; bool hasEditor() const override { return true; }
 const juce::String getName() const override { return JucePlugin_Name; }
 bool acceptsMidi() const override { return false; } bool producesMidi() const override { return false; }
 bool isMidiEffect() const override { return false; } double getTailLengthSeconds() const override { return 0.0; }
 int getNumPrograms() override { return 1; } int getCurrentProgram() override { return 0; }
 void setCurrentProgram(int) override {} const juce::String getProgramName(int) override { return {}; }
 void changeProgramName(int, const juce::String&) override {}
 void getStateInformation(juce::MemoryBlock&) override; void setStateInformation(const void*, int) override;
 juce::AudioProcessorValueTreeState parameters;
 float getInputPeak() const noexcept { return inputPeak.load(); }
 float getOutputPeak() const noexcept { return outputPeak.load(); }
 float getInputRms() const noexcept { return inputRms.load(); }
 float getOutputRms() const noexcept { return outputRms.load(); }
private:
 static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
 juce::AudioBuffer<float> dryBuffer;
 std::array<std::unique_ptr<juce::dsp::Oversampling<float>>, 3> oversamplers;
 juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> inputGain, outputGain, attackAmount, sustainAmount, punchAmount, wetMix;
 float fastEnvelope = 0.0f, slowEnvelope = 0.0f; double currentSampleRate = 44100.0;
 int lastProcessingMode = -1, activeLatencySamples = 0;
 std::atomic<float> inputPeak { 0.0f }, outputPeak { 0.0f }, inputRms { 0.0f }, outputRms { 0.0f };
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveBombzPUNCHAudioProcessor)
};
