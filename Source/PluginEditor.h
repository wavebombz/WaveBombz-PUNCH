#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
class WaveBombzPUNCHAudioProcessorEditor final:public juce::AudioProcessorEditor,private juce::Timer {
public: explicit WaveBombzPUNCHAudioProcessorEditor(WaveBombzPUNCHAudioProcessor&);
 void paint(juce::Graphics&)override;void resized()override;
private:
 void timerCallback()override;void setup(juce::Slider&,juce::Label&,const juce::String&);
 WaveBombzPUNCHAudioProcessor& processor;juce::Label title,subtitle,meter;
 juce::Slider attack,sustain,punch,mix,input,output;juce::Label labels[6];
 juce::ToggleButton clip{"SOFT CLIP"};juce::ComboBox os;
 std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachments[6];
 std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> clipAttachment;
 std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> osAttachment;
 float inLevel=0,outLevel=0;JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveBombzPUNCHAudioProcessorEditor)
};
