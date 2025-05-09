#include "PluginProcessor.h"
#include "PluginEditor.h"

// LookAndFeel Implementation
void LookAndFeel::drawRotarySlider(juce::Graphics &g,
                                   int x,
                                   int y,
                                   int width,
                                   int height,
                                   float sliderPosProportional,
                                   float rotaryStartAngle,
                                   float rotaryEndAngle,
                                   juce::Slider &slider)
{
    using namespace juce;
    
    
    auto bounds = Rectangle<float>(x, y, width, height);
    
    auto enabled = slider.isEnabled();

    g.setColour(enabled ? Colour(236u, 236u, 231u) : Colours::grey);
    g.fillEllipse(bounds);

    g.setColour(enabled ? Colour(54u, 69u, 79u) : Colours::grey);
    g.drawEllipse(bounds, 1.f);

    if (auto* rswl = dynamic_cast<RotarySliderWithLabels*>(&slider))
    {
        auto center = bounds.getCentre();
        Path p;

        Rectangle<float> r;
        r.setLeft(center.getX() - 2);
        r.setRight(center.getX() + 2);
        r.setTop(bounds.getY());
        r.setBottom(center.getY() - rswl->getTextHeight() * 1.5);

        p.addRoundedRectangle(r, 2.f);

        jassert(rotaryStartAngle < rotaryEndAngle);

        auto sliderAngRad = jmap(sliderPosProportional, 0.f, 1.f, rotaryStartAngle, rotaryEndAngle);
        p.applyTransform(AffineTransform().rotated(sliderAngRad, center.getX(), center.getY()));

        g.fillPath(p);

        g.setFont(rswl->getTextHeight());
        auto text = rswl->getDisplayString();
        auto strWidth = g.getCurrentFont().getStringWidth(text);

        r.setSize(strWidth + 4, rswl->getTextHeight() + 2);
        r.setCentre(bounds.getCentre());

        g.setColour(Colours::transparentWhite);
        g.fillRect(r);

        g.setColour(enabled ? Colours::black : Colours::white);
        g.drawFittedText(text, r.toNearestInt(), juce::Justification::centred, 1);
    }
}

// Add the missing function definition for the toggle button
void LookAndFeel::drawToggleButton(juce::Graphics &g,
                                   juce::ToggleButton &toggleButton,
                                   bool shouldDrawButtonAsHighlighted,
                                   bool shouldDrawButtonAsDown)
{
    using namespace juce;

    Path powerButton;

    auto bounds = toggleButton.getLocalBounds();
    auto size = jmin(bounds.getWidth(), bounds.getHeight()) - 10;
    auto r = bounds.withSizeKeepingCentre(size, size).toFloat();

    float ang = 40.f;

    size -= 8;

    powerButton.addCentredArc(r.getCentreX(),
                              r.getCentreY(),
                              size * 0.5,
                              size * 0.5,
                              0.f,
                              degreesToRadians(ang),
                              degreesToRadians(360.f - ang),
                              true);

    powerButton.startNewSubPath(r.getCentreX(), r.getY());
    powerButton.lineTo(r.getCentre());

    PathStrokeType pst(2.f, PathStrokeType::curved);

    auto color = toggleButton.getToggleState() ? Colours::grey : Colours::black;
    
    g.setColour(color);
    g.strokePath(powerButton, pst);
    g.drawEllipse(r, 2);
}



//============================================================================

void RotarySliderWithLabels::paint(juce::Graphics &g)
{
    using namespace juce;
    
    auto startAng = degreesToRadians(180.f + 45.f);
    auto endAng = degreesToRadians(180.f - 45.f) + MathConstants<float>::twoPi;
    
    auto range = getRange();
    auto sliderBounds = getSliderBounds();
    
//    g.setColour(Colours::red);  // Debugging rectangle
//    g.drawRect(getLocalBounds());
//    g.setColour(Colours::yellow);  // Debugging slider bounds
//    g.drawRect(sliderBounds);

    getLookAndFeel().drawRotarySlider(g,
                                      sliderBounds.getX(),
                                      sliderBounds.getY(),
                                      sliderBounds.getWidth(),
                                      sliderBounds.getHeight(),
                                      jmap(getValue(), range.getStart(), range.getEnd(), 0.0, 1.0),
                                      startAng,
                                      endAng,
                                      *this);

    auto center = sliderBounds.toFloat().getCentre();
    auto radius = sliderBounds.getWidth() * 0.5f;

    g.setColour(Colours::black);
    g.setFont(getTextHeight());

    auto numChoices = labels.size();
    for (int i = 0; i < numChoices; ++i )
    {
        auto pos = labels[i].pos;
        jassert(0.f <= pos);
        jassert(pos <= 1.f);
        
        auto ang = jmap(pos, 0.f, 1.f, startAng, endAng);
        auto c = center.getPointOnCircumference(radius + getTextHeight() * 1.2f, ang);

        Rectangle<float> r;
        auto str = labels[i].label;
        r.setSize(g.getCurrentFont().getStringWidth(str), getTextHeight());
        r.setCentre(c);

        g.drawFittedText(str, r.toNearestInt(), juce::Justification::centred, 1);
    }
}

juce::Rectangle<int> RotarySliderWithLabels::getSliderBounds() const   // Corrected scope resolution
{
    //return getLocalBounds();  // Adjust if needed for proper bounds
    auto bounds = getLocalBounds();
    
    auto size = juce::jmin(bounds.getWidth(), bounds.getHeight());
    
    size -= getTextHeight() * 2;
    juce::Rectangle<int> r;
    r.setSize(size, size);
    r.setCentre(bounds.getCentreX(), 0);
    r.setY(2);
    
    return r;
    
}


juce::String RotarySliderWithLabels::getDisplayString() const
{
    if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(param) )
        return choiceParam->getCurrentChoiceName();
    
    juce::String str;
    bool addK = false;
    
    if (auto* floatParam = dynamic_cast<juce::AudioParameterFloat*>(param) )
    {
        float val = getValue();
        if (val > 999.f)
        {
            val /= 1000.f;
            addK = true;
        }
        
        str = juce::String(val, (addK ? 2 : 0));
    }
    else {
        jassertfalse;
    }
        
    if (suffix.isNotEmpty() )
    {
        str << " ";
        if (addK )
            str << "k";
        
        str << suffix;
    }
    
    return str;
    
    }

//============================================================================

ResponseCurveComponent::ResponseCurveComponent(ColinasEQAudioProcessor& p) :
    audioProcessor(p),
    //leftChannelFifo(&audioProcessor.leftChannelFifo)
    
leftPathProducer(audioProcessor.leftChannelFifo),
rightPathProducer(audioProcessor.rightChannelFifo)

{
    const auto& params = audioProcessor.getParameters();
    for (auto param :  params)
    {
        param->addListener(this);
    }

    
    
    updateChain();
    
    startTimerHz(60);
}

ResponseCurveComponent::~ResponseCurveComponent()
{
    const auto& params = audioProcessor.getParameters();
    for( auto param : params)
    {
        param->removeListener(this);
    }
}

void ResponseCurveComponent::parameterValueChanged(int parameterIndex, float newValue)
{
    parametersChanged.set(true);
}

void PathProducer::process(juce::Rectangle<float> fftBounds, double sampleRate)
{
    juce::AudioBuffer<float> tempIncomingBuffer;
    
    while (leftChannelFifo->getNumCompleteBuffersAvailable() > 0)
    {
        if (leftChannelFifo->getAudioBuffer(tempIncomingBuffer) )
        {
            auto size = tempIncomingBuffer.getNumSamples();
            
            juce::FloatVectorOperations::copy(monoBuffer.getWritePointer(0, 0),
                                              monoBuffer.getReadPointer(0, size),
                                              monoBuffer.getNumSamples() - size);
            
            juce::FloatVectorOperations::copy(monoBuffer.getWritePointer(0, monoBuffer.getNumSamples() - size), tempIncomingBuffer.getReadPointer(0,0),
                                              size);
            
            leftChannelFFTDataGenerator.produceFFTDataForRendering(monoBuffer, -48.f);
        }
    }
    
    const auto fftSize = leftChannelFFTDataGenerator.getFFTSize();
    
    const auto binWidth = sampleRate / (double)fftSize;
    
    while (leftChannelFFTDataGenerator.getNumAvailableFFTDataBlocks() > 0)
    {
        std::vector<float> fftData;
        if (leftChannelFFTDataGenerator.getFFTData(fftData) )
        {
            pathProducer.generatePath(fftData, fftBounds, fftSize, binWidth, -48.f);
        }
    }
    
    while (pathProducer.getNumPathsAvailable() )
    {
        pathProducer.getPath(leftChannelFFTPath);
    }
}

void ResponseCurveComponent::timerCallback()
{
    auto fftBounds = getAnalysisArea().toFloat();
    auto sampleRate = audioProcessor.getSampleRate();
    
    leftPathProducer.process(fftBounds, sampleRate);
    rightPathProducer.process(fftBounds, sampleRate);
    
    if( parametersChanged.compareAndSetBool(false, true) )
    {
        DBG( "params changed");
        //update the monochain
        updateChain();
        //signal a repaint
        //repaint();
    }
    
    repaint();

}

void ResponseCurveComponent::updateChain()
{
    auto chainSettings = getChainSettings(audioProcessor.apvts);
    
    monoChain.setBypassed<ChainPositions::LowCut>(chainSettings.lowCutBypassed);
    monoChain.setBypassed<ChainPositions::Peak>(chainSettings.peakBypassed);
    monoChain.setBypassed<ChainPositions::HighCut>(chainSettings.highCutBypassed);


    
    auto peakCoefficients = makePeakFilter(chainSettings, audioProcessor.getSampleRate());
    updateCoefficients(monoChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);
    
    auto lowCutCoefficients = makeLowCutFilter(chainSettings, audioProcessor.getSampleRate());
    auto highCutCoefficients = makeHighCutFilter(chainSettings, audioProcessor.getSampleRate());
    
    updateCutFilter(monoChain.get<ChainPositions::LowCut>(), lowCutCoefficients, chainSettings.lowCutSlope);
    updateCutFilter(monoChain.get<ChainPositions::HighCut>(), highCutCoefficients, chainSettings.highCutSlope);
    
}


void ResponseCurveComponent::paint (juce::Graphics& g)
{
   using namespace juce;
   // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (Colours::black);
    
    g.drawImage(background, getLocalBounds().toFloat());

    auto responseArea = getAnalysisArea();
    
    auto w = responseArea.getWidth();

    auto& lowcut = monoChain.get<ChainPositions::LowCut>();
    auto& peak = monoChain.get<ChainPositions::Peak>();
    auto& highcut = monoChain.get<ChainPositions::HighCut>();

    auto sampleRate = audioProcessor.getSampleRate();

    std::vector<double> mags;

    mags.resize(w);


   for( int i = 0; i < w; ++i)
   {
       double mag = 1.f;
       auto freq = mapToLog10(double(i) / double(w),20.0, 20000.0);
       
       if(! monoChain.isBypassed<ChainPositions::Peak>() )
           mag *= peak.coefficients->getMagnitudeForFrequency(freq, sampleRate);
       
       if( !monoChain.isBypassed<ChainPositions::LowCut>() )
       {
           if(! lowcut.isBypassed<0>() )
               mag *= lowcut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
           if(! lowcut.isBypassed<1>() )
               mag *= lowcut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
           if(! lowcut.isBypassed<2>() )
               mag *= lowcut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
           if(! lowcut.isBypassed<3>() )
               mag *= lowcut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
       }
       
       
       
       if( !monoChain.isBypassed<ChainPositions::HighCut>() )
       {
           if(! highcut.isBypassed<0>() )
               mag *= highcut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
           if(! highcut.isBypassed<1>() )
               mag *= highcut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
           if(! highcut.isBypassed<2>() )
               mag *= highcut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
           if(! highcut.isBypassed<3>() )
               mag *= highcut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
       }
       
       
       
       mags[i] =  Decibels::gainToDecibels(mag);

   }
   Path responseCurve;
   
   const double outputMin = responseArea.getBottom();
   const double outputMax = responseArea.getY();
   auto map = [outputMin, outputMax] ( double input)
   {
       return jmap(input, -24.0, 24.0, outputMin, outputMax);
   };
   
   responseCurve.startNewSubPath(responseArea.getX(), map(mags.front()));
   
   for( size_t i = 1; i < mags.size(); ++i)
   {
       responseCurve.lineTo(responseArea.getX() + i, map(mags[i]));
   }
    
    auto leftChannelFFTPath = leftPathProducer.getPath();
    
    leftChannelFFTPath.applyTransform(AffineTransform().translation(responseArea.getX(), responseArea.getY()));
    
    g.setColour(Colours::orange); // Spectrum Analyzer Colour
    g.strokePath(leftChannelFFTPath, PathStrokeType(3.f));
    
    auto rightChannelFFTPath = rightPathProducer.getPath();
    rightChannelFFTPath.applyTransform(AffineTransform().translation(responseArea.getX(), responseArea.getY()));
    
    g.setColour(Colours::yellow); // Spectrum Analyzer Colour
    g.strokePath(rightChannelFFTPath, PathStrokeType(3.f));

    
    
    g.setColour(Colours::lightblue);
    g.drawRoundedRectangle(getRenderArea().toFloat(), 8.f, 4.f);//Size of the Rectangle of the response curve
   
    g.setColour(Colours::lightblue);
    g.strokePath(responseCurve, PathStrokeType(4.f));//Size of the response curve
   
}

void ResponseCurveComponent::resized()
{
    using namespace juce;
    background = Image(Image::PixelFormat::RGB, getWidth(), getHeight(), true);
    
    Graphics g(background);
    
    Array<float> freqs
    {
        20, /*30, 40,*/ 50, 100,
        200, /*300, 400,*/ 500, 1000,
        2000, /*3000, 4000,*/ 5000, 10000,
        20000
    };
    
    
    auto renderArea = getAnalysisArea();
    auto left = renderArea.getX();
    auto right = renderArea.getRight();
    auto top = renderArea.getY();
    auto bottom = renderArea.getBottom();
    auto width = renderArea.getWidth();
    
    Array<float> xs;
    for ( auto f : freqs )
    {
        auto normX = mapFromLog10(f, 20.f, 20000.f);
        xs.add(left + width * normX);
    }
    
    g.setColour(Colours::lightblue);
    //for(auto f : freqs)
    for (auto x : xs)
    {
        //auto normX = mapFromLog10(f, 20.f, 20000.f);
        
        //g.drawVerticalLine(getWidth() * normX, 0.f, getHeight());
        
        g.drawVerticalLine(x, top, bottom);
    }
    
    Array<float> gain
    {
        -24, -12, 0, 12, 24
    };
    
    for(auto gDb : gain)
    {
        auto y = jmap(gDb, -24.f, 24.f, float(bottom), float(top));
        //g.drawHorizontalLine(y, 0, getWidth());
        g.setColour(gDb == 0.f ? Colours::orange : Colours::darkgrey);
        g.drawHorizontalLine(y, left, right);
    }
    
   // g.drawRect(getAnalysisArea());
    
    g.setColour(Colours::lightblue);
    const int fontHeight = 13;
    g.setFont(fontHeight);
    
    for (int i = 0; i < freqs.size(); ++i )
    {
        auto f = freqs[i];
        auto x = xs[i];
        
        bool addK = false;
        String str;
        if ( f > 999.f )
        {
            addK = true;
            f /= 1000.f;
        }
        
        str << f;
        if( addK)
            str << "k";
        str << "Hz";
        
        auto textWidth = g.getCurrentFont().getStringWidth(str);
        
        
        Rectangle<int> r;
        r.setSize(textWidth, fontHeight);
        r.setCentre(x, 0);
        r.setY(1);
        
        g.drawFittedText(str, r, juce::Justification::centred, 1);
        
    }
    
    for ( auto gDb : gain )
    {
        auto y = jmap(gDb, -24.f, 24.f, float(bottom), float(top));
        
        String str;
        if ( gDb > 0)
            str << "+";
        str << gDb;
        
        auto textWidth = g.getCurrentFont().getStringWidth(str);
        
        Rectangle<int> r;
        r.setSize(textWidth, fontHeight);
        r.setX(getWidth() - textWidth);
        r.setCentre(r.getCentreX(), y);
        
        g.setColour(gDb == 0.f ? Colours::orange : Colours::lightblue);
        
        g.drawFittedText(str, r, juce::Justification::centred, 1);
        
        str.clear();
        str << (gDb - 24.f);
        
        r.setX(1);
        textWidth = g.getCurrentFont().getStringWidth(str);
        r.setSize(textWidth, fontHeight);
        g.setColour(Colours::lightblue);
        g.drawFittedText(str, r, juce::Justification::centred, 1);
        
    }
    
    
    
}

juce::Rectangle<int> ResponseCurveComponent::getRenderArea()
{
    auto bounds = getLocalBounds();
    
    //bounds.reduce(10, 8);
    
    bounds.removeFromTop(20);
    bounds.removeFromBottom(5);
    bounds.removeFromLeft(30);
    bounds.removeFromRight(30);
    
    return bounds;
    
    
}


juce::Rectangle<int> ResponseCurveComponent::getAnalysisArea()
{
    auto bounds = getRenderArea();
    bounds.removeFromTop(4);
    bounds.removeFromBottom(4);
    return bounds;
}



//==============================================================================
    ColinasEQAudioProcessorEditor::ColinasEQAudioProcessorEditor (ColinasEQAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),


    peakFreqSlider(*audioProcessor.apvts.getParameter("Peak Freq"),"Hz"),
    peakGainSlider(*audioProcessor.apvts.getParameter("Peak Gain"), "dB"),
    peakQualitySlider(*audioProcessor.apvts.getParameter("Peak Quality"),""),
    lowCutFreqSlider(*audioProcessor.apvts.getParameter("LowCut Freq"), "Hz"),
    highCutFreqSlider(*audioProcessor.apvts.getParameter("HighCut Freq"), "Hz"),
    lowCutSlopeSlider(*audioProcessor.apvts.getParameter("LowCut Slope"), "dB/Oct"),
    highCutSlopeSlider(*audioProcessor.apvts.getParameter("HighCut Slope"), "dB/Oct"),

    responseCurveComponent(audioProcessor),
    peakFreqSliderAttachment(audioProcessor.apvts, "Peak Freq", peakFreqSlider),
    peakGainSliderAttachment(audioProcessor.apvts, "Peak Gain", peakGainSlider),
    peakQualitySliderAttachment(audioProcessor.apvts, "Peak Quality", peakQualitySlider),
    lowCutFreqSliderAttachment(audioProcessor.apvts, "LowCut Freq", lowCutFreqSlider),
    highCutFreqSliderAttachment(audioProcessor.apvts, "HighCut Freq", highCutFreqSlider),
    lowCutSlopeSliderAttachment(audioProcessor.apvts, "LowCut Slope", lowCutSlopeSlider),
    highCutSlopeSliderAttachment(audioProcessor.apvts, "HighCut Slope", highCutSlopeSlider),
    


    lowcutBypassButtonAttachment(audioProcessor.apvts, "LowCut Bypassed",lowcutBypassButton),
    peakBypassButtonAttachment(audioProcessor.apvts,"Peak Bypassed", peakBypassButton),
    highcutBypassButtonAttachment(audioProcessor.apvts, "HighCut Bypassed",highcutBypassButton),
    analyzerEnabledButtonAttachment(audioProcessor.apvts, "Analyzer Enabled",analyzerEnabledButton)

{
    
        peakFreqSlider.labels.add({0.f, "20Hz"});
        peakFreqSlider.labels.add({1.f, "20kHz"});
        
        peakGainSlider.labels.add({0.f, "-24dB"});
        peakGainSlider.labels.add({1.f, "+24dB"});
        
        peakQualitySlider.labels.add({0.f, "0.1"});
        peakQualitySlider.labels.add({1.f, "10.0"});
        
        lowCutFreqSlider.labels.add({0.f, "20Hz"});
        lowCutFreqSlider.labels.add({1.f, "20kHz"});
        
        highCutFreqSlider.labels.add({0.f, "20Hz"});
        highCutFreqSlider.labels.add({1.f, "20kHz"});
        
        lowCutSlopeSlider.labels.add({0.0f, "12"});
        lowCutSlopeSlider.labels.add({1.f, "48"});
        
        highCutSlopeSlider.labels.add({0.0f, "12"});
        highCutSlopeSlider.labels.add({1.f, "48"});
    
    for( auto* comp : getComps() )
    {
        addAndMakeVisible(comp);
    }
    
    peakBypassButton.setLookAndFeel(&lnf);
    lowcutBypassButton.setLookAndFeel(&lnf);
    highcutBypassButton.setLookAndFeel(&lnf);
    
    auto safePtr = juce::Component::SafePointer<ColinasEQAudioProcessorEditor>(this);
    peakBypassButton.onClick = [safePtr]()
    {
        if(auto* comp = safePtr.getComponent() )
        {
            auto bypassed = comp->peakBypassButton.getToggleState();
            
            comp->peakFreqSlider.setEnabled(!bypassed);
            comp->peakGainSlider.setEnabled(!bypassed);
            comp->peakQualitySlider.setEnabled(!bypassed);
        }
    };
    
    lowcutBypassButton.onClick = [safePtr]()
    {
        if (auto* comp =safePtr.getComponent() )
        {
            auto bypassed = comp->lowcutBypassButton.getToggleState();
            comp->lowCutFreqSlider.setEnabled( !bypassed);
            comp->lowCutSlopeSlider.setEnabled(!bypassed);
        }
    };
    
    
    highcutBypassButton.onClick = [safePtr]()
    {
        if (auto* comp =safePtr.getComponent() )
        {
            auto bypassed = comp->highcutBypassButton.getToggleState();
            comp->highCutFreqSlider.setEnabled( !bypassed);
            comp->highCutSlopeSlider.setEnabled(!bypassed);

        }
    };
    
    setSize (800, 600);
}

ColinasEQAudioProcessorEditor::~ColinasEQAudioProcessorEditor()
{
 
}

//==============================================================================
 void ColinasEQAudioProcessorEditor::paint (juce::Graphics& g)
{
    using namespace juce;
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (Colours::lightblue);
     
//     g.setColour (juce::Colours::black);
//     g.setFont (13.0f);
//     g.drawFittedText ("Peak", getLocalBounds(), juce::Justification::centredBottom, 20);
//     
//     Rectangle<float> bounds { 16.0f, 100.0f, 160.0f, 295.0f };
//     g.setColour (Colours::lightblue);
//     g.fillRect (bounds);
//     AttributedString str ("Low Cut");
//     str.setJustification (Justification::centredBottom);
//     str.setColour (Colours::black);
//     str.draw (g, bounds);
//     
//     Rectangle<float> Bounds { 427.0f, 100.0f, 100.0f, 295.0f };
//     g.setColour(Colours::lightblue);
//     g.fillRect (Bounds);
//     AttributedString Str ("High Cut");
//     Str.setJustification (Justification::bottomRight);
//     Str.setColour (Colours::black);
//     Str.draw (g, Bounds);
     
     
}

void ColinasEQAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
//    float hRatio = 33.f / 100.f;
    auto responseArea = bounds.removeFromTop(bounds.getHeight() * 0.33);
    
    bounds.removeFromTop(10);
    
    responseCurveComponent.setBounds(responseArea);
    
    auto lowCutArea = bounds.removeFromLeft(bounds.getWidth() * 0.33);
    auto highCutArea = bounds.removeFromRight(bounds.getWidth() * 0.5);
    
    
    lowcutBypassButton.setBounds(lowCutArea.removeFromTop(25));
    lowCutFreqSlider.setBounds(lowCutArea.removeFromTop(highCutArea.getHeight() * 0.5));
    lowCutSlopeSlider.setBounds(lowCutArea);
    
    highcutBypassButton.setBounds(highCutArea.removeFromTop(25));
    highCutFreqSlider.setBounds(highCutArea.removeFromTop(highCutArea.getHeight() * 0.5));
    highCutSlopeSlider.setBounds(highCutArea);
    
    
    peakBypassButton.setBounds(bounds.removeFromTop(25));
    peakFreqSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.33));
    peakGainSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.5));
    peakQualitySlider.setBounds(bounds)
    ;
    
}

std::vector<juce::Component*> ColinasEQAudioProcessorEditor::getComps()
{
    return
    {
        &peakFreqSlider,
        &peakGainSlider,
        &peakQualitySlider,
        &lowCutFreqSlider,
        &highCutFreqSlider,
        &lowCutSlopeSlider,
        &highCutSlopeSlider,
        &responseCurveComponent,
        
        &lowcutBypassButton,
        &peakBypassButton,
        &highcutBypassButton,
        &analyzerEnabledButton
    };
}

 
