#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
ColinasEQAudioProcessor::ColinasEQAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

ColinasEQAudioProcessor::~ColinasEQAudioProcessor()
{
}

//==============================================================================
const juce::String ColinasEQAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool ColinasEQAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool ColinasEQAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool ColinasEQAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double ColinasEQAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int ColinasEQAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int ColinasEQAudioProcessor::getCurrentProgram()
{
    return 0;
}

void ColinasEQAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String ColinasEQAudioProcessor::getProgramName (int index)
{
    return {};
}

void ColinasEQAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void ColinasEQAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    
    /** The ProcessSpec object passes the filter signal to the left and right chain  */
    juce::dsp::ProcessSpec spec;
    
    spec.maximumBlockSize = samplesPerBlock;
    
    spec.numChannels = 1;
    
    spec.sampleRate = sampleRate;
    
    leftChain.prepare(spec);
    rightChain.prepare(spec);
    
    updateFilters();
    
    leftChannelFifo.prepare(samplesPerBlock);
    rightChannelFifo.prepare(samplesPerBlock);
    
    osc.initialise([](float x) { return std::sin(x); });
    
    spec.numChannels = getTotalNumOutputChannels();
    osc.prepare(spec);
    osc.setFrequency(5000);

}

void ColinasEQAudioProcessor::releaseResources()
{

}

#ifndef JucePlugin_PreferredChannelConfigurations
bool ColinasEQAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void ColinasEQAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    updateFilters();
    
    juce::dsp::AudioBlock<float> block(buffer);
    
//    testing the spectrum analyzer with sound waves
//    buffer.clear();
//    
//    juce::dsp::ProcessContextReplacing<float> stereoContext(block);
//    osc.process(stereoContext);
    
    
    
    
    /** An AudioBlock is required when creating a ProcessContextReplacing.
        @see ProcessContextReplacing */
    
    auto leftBlock = block.getSingleChannelBlock(0);
    auto rightBlock = block.getSingleChannelBlock(1);
    
    /** The process chain requires a ProcessContextReplacing to be passed to it in order to run the links in the chain */
    juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
    juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);
    
    /** Passes the context to the mono filter chains*/
    leftChain.process(leftContext);
    rightChain.process(rightContext);
    
    leftChannelFifo.update(buffer);
    rightChannelFifo.update(buffer);
}

//==============================================================================
bool ColinasEQAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* ColinasEQAudioProcessor::createEditor()
{
    return new ColinasEQAudioProcessorEditor (*this);
    //return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void ColinasEQAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::MemoryOutputStream mos(destData, true);
    apvts.state.writeToStream(mos);
    
}

void ColinasEQAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto tree = juce::ValueTree::readFromData(data, sizeInBytes);
    if (tree.isValid() )
    {
        apvts.replaceState(tree);
        updateFilters();
    }
}


ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts)
{
    ChainSettings settings;
    
    /**
     @param the apvts.getRawParameterValue function gets the parameters values from the AudioProcessorValueTreeState
            in units within the ranges defined by the parameters
     */
    settings.lowCutFreq = apvts.getRawParameterValue("LowCut Freq")->load();
    settings.highCutFreq = apvts.getRawParameterValue("HighCut Freq")->load();
    settings.peakFreq = apvts.getRawParameterValue("Peak Freq")->load();
    settings.peakGainDecibels = apvts.getRawParameterValue("Peak Gain")->load();
    settings.peakQuality = apvts.getRawParameterValue("Peak Quality")->load();
    settings.lowCutSlope = static_cast<Slope> (apvts.getRawParameterValue("LowCut Slope")->load());
    settings.highCutSlope = static_cast<Slope>(apvts.getRawParameterValue("HighCut Slope")->load());
    
    settings.lowCutBypassed = apvts.getRawParameterValue("LowCut Bypassed")->load() > 0.5f;
    settings.peakBypassed = apvts.getRawParameterValue("Peak Bypassed")->load() > 0.5f;
    settings.highCutBypassed = apvts.getRawParameterValue("HighCut Bypassed")->load() > 0.5f;

    return settings;
}


Coefficients makePeakFilter(const ChainSettings& chainSettings, double sampleRate)
{
    return juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate,
                                                               chainSettings.peakFreq,
                                                               chainSettings.peakQuality,
                                                               juce::Decibels::decibelsToGain(chainSettings.peakGainDecibels));
}

void ColinasEQAudioProcessor::updatePeakFilter(const ChainSettings &chainSettings)
{
    auto peakCoefficients = makePeakFilter(chainSettings, getSampleRate());
    
    leftChain.setBypassed<ChainPositions::Peak>(chainSettings.peakBypassed);
    rightChain.setBypassed<ChainPositions::Peak>(chainSettings.peakBypassed);

    
    updateCoefficients(leftChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);
    updateCoefficients(rightChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);

}

void updateCoefficients(Coefficients &old, const Coefficients &replacements)
{
    *old = *replacements;
};

void ColinasEQAudioProcessor::updateLowCutFilters(const ChainSettings &chainSettings)
{
    // Check if the low-cut frequency is valid
//    if (chainSettings.lowCutFreq <= 0 || chainSettings.lowCutFreq > getSampleRate() * 0.5)
//    {
//        DBG("Low cut frequency out of range: " << chainSettings.lowCutFreq);
//        return; // Exit the function if the frequency is out of range
//    }

    auto cutCoefficients = makeLowCutFilter(chainSettings, getSampleRate());
    auto& leftLowCut = leftChain.get<ChainPositions::LowCut>();
    auto& rightLowCut = rightChain.get<ChainPositions::LowCut>();
    
    leftChain.setBypassed<ChainPositions::LowCut>(chainSettings.lowCutBypassed);
    rightChain.setBypassed<ChainPositions::LowCut>(chainSettings.lowCutBypassed);

    
    updateCutFilter(leftLowCut, cutCoefficients, chainSettings.lowCutSlope);
    updateCutFilter(rightLowCut, cutCoefficients, chainSettings.lowCutSlope);
}


void ColinasEQAudioProcessor::updateHighCutFilters(const ChainSettings &chainSettings)
{
//    // Check if the high-cut frequency is valid
//    if (chainSettings.highCutFreq <= 0 || chainSettings.highCutFreq > getSampleRate() * 0.5)
//    {
//        DBG("High cut frequency out of range: " << chainSettings.highCutFreq);
//        return; // Exit the function if the frequency is out of range
//    }

    auto highCutCoefficients = makeHighCutFilter(chainSettings, getSampleRate());
    auto& leftHighCut = leftChain.get<ChainPositions::HighCut>();
    auto& rightHighCut = rightChain.get<ChainPositions::HighCut>();
    
    
    leftChain.setBypassed<ChainPositions::HighCut>(chainSettings.highCutBypassed);
    rightChain.setBypassed<ChainPositions::HighCut>(chainSettings.highCutBypassed);


    updateCutFilter(leftHighCut, highCutCoefficients, chainSettings.highCutSlope);
    updateCutFilter(rightHighCut, highCutCoefficients, chainSettings.highCutSlope);
}


void ColinasEQAudioProcessor::updateFilters()
{
    auto chainSettings = getChainSettings(apvts);
    
    updateLowCutFilters(chainSettings);
    updatePeakFilter(chainSettings);
    updateHighCutFilters(chainSettings);
    
}

/** Declaration of the apvts object.
    The AudioProcessParameter juce class is inherited. */
juce::AudioProcessorValueTreeState::ParameterLayout ColinasEQAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    /** The AudioParameterFloat is a derived type juce class that represents the sliders on the GUI.
        @see juce::NormalisableRange
     */
    layout.add(std::make_unique<juce::AudioParameterFloat>("LowCut Freq",
                                                           "LowCut Freq",
                                                           /** Sets the frequency range and the changes the value of the sliders,  */
                                                           juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f), 20.f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>("HighCut Freq",
                                                           "HighCut Freq",
                                                           juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f), 20000.f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>("Peak Freq",
                                                           "Peak Freq",
                                                           juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f), 750.f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>("Peak Gain",
                                                           "Peak Gain",
                                                           juce::NormalisableRange<float>(-24.f, 24.f, 0.5f, 1.f), 0.0f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>("Peak Quality",
                                                           "Peak Quality",
                                                           juce::NormalisableRange<float>(0.1f, 10.f, 0.05f, 1.f), 1.f));
    /** Creation of a string array
     @see AudioParameterChoice */
    juce::StringArray stringArray;
    for( int i = 0; i < 4; i++)
    {
        juce::String str;
        str << (12 + i*12);
        str << " db/Oct";
        stringArray.add(str);
    }
    
    /** The slope cutoff is measured in decibles of gain reduction per octave, therefore, the AudioParameterChoice was necessary since it was needed
     to choose between 4 options instead of a frequecy range values like the above parameters */
    layout.add(std::make_unique<juce::AudioParameterChoice>("LowCut Slope", "LowCut Slope", stringArray, 0));
    layout.add(std::make_unique<juce::AudioParameterChoice>("HighCut Slope", "HighCut Slope", stringArray, 0));

    
    layout.add(std::make_unique<juce::AudioParameterBool>("LowCut Bypassed", "LowCut Bypassed", false));
    layout.add(std::make_unique<juce::AudioParameterBool>("Peak Bypassed", "Peak Bypassed", false));
    layout.add(std::make_unique<juce::AudioParameterBool>("HighCut Bypassed", "HighCut Bypassed", false));
    layout.add(std::make_unique<juce::AudioParameterBool>("Analyzer Enable", "Analyzer Enable", true));



    
    return layout;
};


//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ColinasEQAudioProcessor();
}

