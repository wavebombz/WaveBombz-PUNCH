#include "PluginProcessor.h"
#include "PluginEditor.h"
namespace { float dbToGain(float db) noexcept { return std::pow(10.0f, db / 20.0f); } }
WaveBombzPUNCHAudioProcessor::WaveBombzPUNCHAudioProcessor()
 : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
 .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
 parameters(*this, nullptr, "PUNCH_STATE", createParameterLayout()) {}
juce::AudioProcessorValueTreeState::ParameterLayout WaveBombzPUNCHAudioProcessor::createParameterLayout() {
 using F=juce::AudioParameterFloat; juce::AudioProcessorValueTreeState::ParameterLayout p;
 p.add(std::make_unique<F>(juce::ParameterID{"attack",1},"Attack",juce::NormalisableRange<float>{-100,100,0.1f},0));
 p.add(std::make_unique<F>(juce::ParameterID{"sustain",1},"Sustain",juce::NormalisableRange<float>{-100,100,0.1f},0));
 p.add(std::make_unique<F>(juce::ParameterID{"punch",1},"Punch",juce::NormalisableRange<float>{0,100,0.1f},0));
 p.add(std::make_unique<F>(juce::ParameterID{"mix",1},"Mix",juce::NormalisableRange<float>{0,100,0.1f},100));
 p.add(std::make_unique<F>(juce::ParameterID{"input",1},"Input",juce::NormalisableRange<float>{-24,24,0.01f},0));
 p.add(std::make_unique<F>(juce::ParameterID{"output",1},"Output",juce::NormalisableRange<float>{-24,24,0.01f},0));
 p.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"softClip",1},"Soft Clip",true));
 p.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"oversampling",1},"Oversampling",juce::StringArray{"Off","2x","4x","8x"},1));
 return p;
}
void WaveBombzPUNCHAudioProcessor::prepareToPlay(double sr,int maxBlock) {
 currentSampleRate=sr; dryBuffer.setSize(getTotalNumOutputChannels(),maxBlock,false,true,true);
 inputGain.reset(sr,0.02); outputGain.reset(sr,0.02); inputGain.setCurrentAndTargetValue(1); outputGain.setCurrentAndTargetValue(1);
 for(size_t i=0;i<oversamplers.size();++i) {
  const size_t stages=i+1;
  oversamplers[i]=std::make_unique<juce::dsp::Oversampling<float>>(static_cast<size_t>(getTotalNumOutputChannels()),stages,juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,true);
  oversamplers[i]->initProcessing(static_cast<size_t>(maxBlock)); oversamplers[i]->reset();
 }
 fastEnvelope=slowEnvelope=0;
}
void WaveBombzPUNCHAudioProcessor::releaseResources(){ for(auto& o:oversamplers)o.reset(); }
bool WaveBombzPUNCHAudioProcessor::isBusesLayoutSupported(const BusesLayout& l) const {
 const auto in=l.getMainInputChannelSet(),out=l.getMainOutputChannelSet();
 return in==out&&(out==juce::AudioChannelSet::mono()||out==juce::AudioChannelSet::stereo());
}
void WaveBombzPUNCHAudioProcessor::processBlock(juce::AudioBuffer<float>& b,juce::MidiBuffer&) {
 juce::ScopedNoDenormals guard; const int chs=b.getNumChannels(), nS=b.getNumSamples(); if(chs==0||nS==0)return;
 if(dryBuffer.getNumSamples()<nS) { for(int c=0;c<chs;++c) juce::FloatVectorOperations::copy(dryBuffer.getWritePointer(c),b.getReadPointer(c),dryBuffer.getNumSamples()); }
 else dryBuffer.makeCopyOf(b,true);
 inputGain.setTargetValue(dbToGain(parameters.getRawParameterValue("input")->load()));
 outputGain.setTargetValue(dbToGain(parameters.getRawParameterValue("output")->load()));
 const float attack=parameters.getRawParameterValue("attack")->load()*0.01f;
 const float sustain=parameters.getRawParameterValue("sustain")->load()*0.01f;
 const float punch=parameters.getRawParameterValue("punch")->load()*0.01f;
 const float mix=parameters.getRawParameterValue("mix")->load()*0.01f;
 const bool clip=parameters.getRawParameterValue("softClip")->load()>0.5f;
 const int os=juce::jlimit(0,3,static_cast<int>(parameters.getRawParameterValue("oversampling")->load()+0.5f));
 const float fc=std::exp(-1.0f/static_cast<float>(currentSampleRate*0.003));
 const float sc=std::exp(-1.0f/static_cast<float>(currentSampleRate*0.080));
 float peakIn=0,peakOut=0;
 for(int n=0;n<nS;++n) {
  const float ig=inputGain.getNextValue(); float detector=0;
  for(int c=0;c<chs;++c){auto* x=b.getWritePointer(c);x[n]*=ig;detector+=std::abs(x[n]);peakIn=juce::jmax(peakIn,std::abs(x[n]));}
  detector/=static_cast<float>(chs); fastEnvelope=fc*fastEnvelope+(1-fc)*detector; slowEnvelope=sc*slowEnvelope+(1-sc)*detector;
  const float tr=fastEnvelope-slowEnvelope;
  const float shape=juce::jlimit(0.25f,2.5f,1+attack*tr*8)*juce::jlimit(0.25f,2.0f,1+sustain*(slowEnvelope-0.15f))*(1+punch*juce::jmax(0.0f,tr)*5);
  for(int c=0;c<chs;++c)b.getWritePointer(c)[n]*=shape;
 }
 if(clip) {
  if(os==0){for(int c=0;c<chs;++c)for(int n=0;n<nS;++n)b.getWritePointer(c)[n]=std::tanh(b.getWritePointer(c)[n]);}
  else if(oversamplers[static_cast<size_t>(os-1)]) {
   juce::dsp::AudioBlock<float> block(b); auto hi=oversamplers[static_cast<size_t>(os-1)]->processSamplesUp(block);
   for(size_t c=0;c<hi.getNumChannels();++c)for(size_t n=0;n<hi.getNumSamples();++n)hi.getChannelPointer(c)[n]=std::tanh(hi.getChannelPointer(c)[n]);
   oversamplers[static_cast<size_t>(os-1)]->processSamplesDown(block);
  }
 }
 for(int n=0;n<nS;++n){const float og=outputGain.getNextValue();for(int c=0;c<chs;++c){
  auto* w=b.getWritePointer(c);const float dry=(n<dryBuffer.getNumSamples()?dryBuffer.getReadPointer(c)[n]:0.0f);
  w[n]=(dry*(1-mix)+w[n]*mix)*og;peakOut=juce::jmax(peakOut,std::abs(w[n]));
 }}
 inputPeak.store(peakIn);outputPeak.store(peakOut);
}
juce::AudioProcessorEditor* WaveBombzPUNCHAudioProcessor::createEditor(){return new WaveBombzPUNCHAudioProcessorEditor(*this);}
void WaveBombzPUNCHAudioProcessor::getStateInformation(juce::MemoryBlock& d){auto s=parameters.copyState();std::unique_ptr<juce::XmlElement>x(s.createXml());copyXmlToBinary(*x,d);}
void WaveBombzPUNCHAudioProcessor::setStateInformation(const void* d,int n){std::unique_ptr<juce::XmlElement>x(getXmlFromBinary(d,n));if(x&&x->hasTagName(parameters.state.getType()))parameters.replaceState(juce::ValueTree::fromXml(*x));}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new WaveBombzPUNCHAudioProcessor();}
