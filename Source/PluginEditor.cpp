#include "PluginEditor.h"
WaveBombzPUNCHAudioProcessorEditor::WaveBombzPUNCHAudioProcessorEditor(WaveBombzPUNCHAudioProcessor& p):AudioProcessorEditor(&p),processor(p){
 title.setText("WAVEBOMBZ PUNCH",juce::dontSendNotification);title.setFont(juce::Font(juce::FontOptions(25,juce::Font::bold)));title.setColour(juce::Label::textColourId,juce::Colour(0xffffc247));addAndMakeVisible(title);
 subtitle.setText("TRANSIENT SHAPER  /  PUNCH ENHANCER",juce::dontSendNotification);subtitle.setFont(juce::Font(juce::FontOptions(10)));subtitle.setColour(juce::Label::textColourId,juce::Colour(0xff92949a));addAndMakeVisible(subtitle);
 juce::Slider* s[]={&attack,&sustain,&punch,&mix,&input,&output};const char* names[]={"ATTACK","SUSTAIN","PUNCH","MIX","INPUT","OUTPUT"};const char* ids[]={"attack","sustain","punch","mix","input","output"};
 for(int i=0;i<6;++i){setup(*s[i],labels[i],names[i]);attachments[i]=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.parameters,ids[i],*s[i]);}
 clip.setColour(juce::ToggleButton::textColourId,juce::Colour(0xffdedfe2));addAndMakeVisible(clip);clipAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.parameters,"softClip",clip);
 os.addItem("OFF",1);os.addItem("2X",2);os.addItem("4X",3);os.addItem("8X",4);addAndMakeVisible(os);osAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.parameters,"oversampling",os);
 meter.setColour(juce::Label::textColourId,juce::Colour(0xffb8bac0));addAndMakeVisible(meter);
 setResizable(true,true);setResizeLimits(540,360,1100,720);setSize(720,470);startTimerHz(30);
}
void WaveBombzPUNCHAudioProcessorEditor::setup(juce::Slider& s,juce::Label& l,const juce::String& t){
 s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);s.setTextBoxStyle(juce::Slider::TextBoxBelow,false,72,20);
 s.setColour(juce::Slider::rotarySliderFillColourId,juce::Colour(0xffffbd45));s.setColour(juce::Slider::rotarySliderOutlineColourId,juce::Colour(0xff35383e));s.setColour(juce::Slider::textBoxTextColourId,juce::Colours::white);s.setColour(juce::Slider::textBoxBackgroundColourId,juce::Colour(0xff202329));addAndMakeVisible(s);
 l.setText(t,juce::dontSendNotification);l.setJustificationType(juce::Justification::centred);l.setColour(juce::Label::textColourId,juce::Colour(0xffbfc1c6));addAndMakeVisible(l);
}
void WaveBombzPUNCHAudioProcessorEditor::paint(juce::Graphics& g){
 g.fillAll(juce::Colour(0xff15171b));g.setColour(juce::Colour(0xff252930));g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(12),12);
 g.setColour(juce::Colour(0xffffbd45));g.fillRoundedRectangle(24,82,3,42,1.5f);g.setColour(juce::Colour(0xff363a42));g.drawLine(28,139,(float)getWidth()-28,139,1);
 auto r=juce::Rectangle<float>(30,(float)getHeight()-62,(float)getWidth()-60,8);g.setColour(juce::Colour(0xff111317));g.fillRoundedRectangle(r,4);
 g.setColour(juce::Colour(0xffffbd45));g.fillRoundedRectangle(r.withWidth(r.getWidth()*inLevel),4);
 g.setColour(juce::Colour(0xff79c7a2));g.fillRoundedRectangle(r.withY(r.getY()+13).withHeight(5).withWidth(r.getWidth()*outLevel),2.5f);
}
void WaveBombzPUNCHAudioProcessorEditor::resized(){
 auto a=getLocalBounds().reduced(28);title.setBounds(a.removeFromTop(32));subtitle.setBounds(a.removeFromTop(22));a.removeFromTop(24);
 auto row=a.removeFromTop(a.getHeight()-82);const int cell=row.getWidth()/6;juce::Slider* s[]={&attack,&sustain,&punch,&mix,&input,&output};
 for(int i=0;i<6;++i){auto slot=row.removeFromLeft(i==5?row.getWidth():cell);labels[i].setBounds(slot.removeFromTop(22));s[i]->setBounds(slot.reduced(2,0));}
 auto foot=a.removeFromBottom(54);clip.setBounds(foot.removeFromLeft(130).withHeight(28));os.setBounds(foot.removeFromLeft(110).withHeight(28));meter.setBounds(foot.withHeight(28));
}
void WaveBombzPUNCHAudioProcessorEditor::timerCallback(){inLevel=juce::jlimit(0.0f,1.0f,processor.getInputPeak());outLevel=juce::jlimit(0.0f,1.0f,processor.getOutputPeak());meter.setText("IN "+juce::String(juce::Decibels::gainToDecibels(inLevel),1)+" dB   OUT "+juce::String(juce::Decibels::gainToDecibels(outLevel),1)+" dB",juce::dontSendNotification);repaint();}
