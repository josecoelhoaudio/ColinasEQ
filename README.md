# ColinasEQ
A 3-Band EQ with Spectrum Analyzer and Response Curve using JUCE and C++

ColinasEQ is a real-time audio plugin that provides a flexible parametric equalizer using JUCE's DSP framework. Designed for both precision and performance, it features a modular signal chain, dynamic filter configuration, and FIFO-based sample buffering for potential visualization.

Features
- Parametric Peak Filter with customizable frequency, gain, and Q.
- Low Cut & High Cut Filters using multi-slope Butterworth filters (12 dB to 48 dB/oct).
- Real-time Parameter Control using AudioProcessorValueTreeState for automation and state recall.
- Single Channel FIFO Buffering for real-time waveform analysis or visualization (e.g., FFT display).
- Modular Filter Architecture built with juce::dsp::ProcessorChain for clean, extendable design.

Technical Highlights
Uses juce::dsp::FilterDesign for IIR filter coefficient generation.

Filter slope handled via enum-based logic and multistage filter activation.

Custom Fifo and SingleChannelSampleFifo classes enable efficient audio buffering per channel.

Prepared for GUI integration with full parameter binding support.
