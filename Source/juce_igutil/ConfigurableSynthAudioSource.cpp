#include <JuceHeader.h>

#include "ConfigurableSynthAudioSource.h"

#include "juce_igutil/NullProcessor.h"

using namespace std;
using namespace juce;
using namespace juce_igutil;

static const std::string gainParam("gain");

/**
 * Constructor
 * 
 * @param pMTL - logger
 * @param pSynthParameters - value tree for controllable parameters 
 * @param pSynthSound - synth sound to use
 * @param synthVoices - synth voices to use (all voices in the provided 
 *                     container will be added.  We take ownership of the
 *                     objects.)
 * @param keyState - keystate tracker used with onscreen virtual keyboard
 * @param pEffectsProcessor - optional effects processor.  Defaults to a null 
 *                          processor if not specified.
 */
ConfigurableSynthAudioSource::ConfigurableSynthAudioSource(
    std::shared_ptr<juce_igutil::MTLogger> pMTL,
    std::shared_ptr<AudioProcessorValueTreeState> pSynthParameters,
    juce::SynthesiserSound::Ptr pSynthSound,
    std::vector<juce::SynthesiserVoice*> synthVoices,
    juce::MidiKeyboardState & keyState, // todo is this the best place for this?
    std::shared_ptr<Processor> pEffectProcessor
): 
    SynthAudioSource(),
    pSynthParams(pSynthParameters),
    keyboardState (keyState),
    processSpec{0,0,0},
    pFxProcessor(pEffectProcessor)
{
    // synth.addVoice makes a call to prepare() for some reason, so
    // this hack avoids triggering an assert that the samplerate is > 0 there.
    // TODO revisit this; we should move this to the same prepare/render/reset 
    // interface used elsewhere.
    synth.setCurrentPlaybackSampleRate(1);

    for (auto pVoice: synthVoices)
        synth.addVoice( pVoice );

    synth.addSound( pSynthSound );

    // parameters
    pMTL->info("Connecting parameters...");
    pGainParam = pSynthParameters->getRawParameterValue(gainParam);
}

/**
 * Prepare the synth for audio generation
 */
void ConfigurableSynthAudioSource::prepareToPlay(
    const juce::dsp::ProcessSpec & processSpec
)
{
    // init this
    previousGain = *pGainParam;

    synth.setCurrentPlaybackSampleRate (processSpec.sampleRate);

    // prepare all the voices
    for (int i=0; i < synth.getNumVoices(); ++i) {
        SynthesiserVoice * pVoice = synth.getVoice(i);
        pVoice->setCurrentPlaybackSampleRate(processSpec.sampleRate);
    }

    pFxProcessor->prepare(processSpec);
}

/**
 * Render the next block of audio
 */
void ConfigurableSynthAudioSource::renderNextBlock(
    juce::AudioBuffer<float> & outputAudio,
    juce::MidiBuffer & inputMidi,
    int startSample)
{
    // Synths usually need to do this.
    outputAudio.clear();

    keyboardState.processNextMidiBuffer(
        inputMidi, 
        startSample,
        outputAudio.getNumSamples(), 
        true
    );

    synth.renderNextBlock(
        outputAudio, 
        inputMidi,
        startSample,
        outputAudio.getNumSamples()
    );

    //pMTL->debug("ConfigurableSynthAudioSource: gain = "+ String(*pGainParam));// +", waveIndex = "+String(*pWaveIndexParam));

    // create the context for dsp
    dsp::AudioBlock<float> block(outputAudio);
    dsp::ProcessContextReplacing<float> context(block);
    pFxProcessor->process(context);

    // Overall gain
    const float currentGain = *pGainParam;
    if (currentGain == previousGain)
    {
        outputAudio.applyGain(currentGain);
    }
    else
    {
        outputAudio.applyGainRamp(0, outputAudio.getNumSamples(), previousGain, currentGain);
        previousGain = currentGain;
    }
}

/**
 * Release any resources 
 */
void ConfigurableSynthAudioSource::releaseResources() {
    pFxProcessor->reset();
    // note sure if this is needed - TODO test
    //synth.allNotesOff();
}

