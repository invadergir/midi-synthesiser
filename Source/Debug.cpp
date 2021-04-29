#include <JuceHeader.h>
#include "Debug.h"

void debug::checkOutput(const juce::AudioBuffer<float> & outputAudio, std::shared_ptr<juce_igutil::MTLogger> pMTL) {
    //for (int chan = 0; chan < outputAudio.getNumChannels(); ++chan) {
    //    Range<float> range = outputAudio.findMinMax(chan, 0, outputAudio.getNumSamples());
    //    if (range.getStart() < -0.999999f || range.getEnd() > 0.999999f ) {
    //        pMTL->error("ERROR - min/max out of range for this buffer: " + String(range.getStart()) + String(", ") + String(range.getEnd()));
    //    }
    //}
    const float highestAllowed = 0.999999f;
    std::stringstream errors;

    for (int chan = 0; chan < outputAudio.getNumChannels(); ++chan) {
        const float * p = outputAudio.getReadPointer(chan);
        for (int ix=0; ix<outputAudio.getNumSamples(); ++ix, ++p) {
            if ( *p < -highestAllowed || *p > highestAllowed ) {
                errors << "ERROR: sample out of bounds (" << *p << "), channel (" <<chan<< "), index (" << ix << ")" << std::endl;
            }
        }
    }
    std::string errorsStr = errors.str();
    if ( ! errorsStr.empty() ) {
        pMTL->debug(juce::String(errorsStr));
    }
}

