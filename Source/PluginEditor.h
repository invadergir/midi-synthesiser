/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "ScopeComponent.h"


//==============================================================================
/**
*/
class MidisynthesiserAudioProcessorEditor : 
    public juce::AudioProcessorEditor,
    private juce::Timer
{
public:

    MidisynthesiserAudioProcessorEditor(
        MidisynthesiserAudioProcessor & p,
        std::shared_ptr<juce::AudioProcessorValueTreeState> pSynthParams
    );
    ~MidisynthesiserAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

    // TODO Why is this needed?
    void timerCallback() override
    {
        keyboardComponent.grabKeyboardFocus();
        stopTimer();
    }

private:

    // helper to set the knob style the same for every control knob
    inline void setKnobStyle(juce::Slider & knob);
    
    // Helper to align a label to a knob in a standard way
    inline void alignLabelToKnob(
        juce::Label & label, 
        juce::Slider & knob, 
        const int numLines = 1);

    // callback when a combobox for the effect type changes
    void changedEffectType(const int fxGroupIndex);

    // get the values for each combobox type selector param, then set the combobox
    //void updateComboBoxesFromParams();

    /**
     * Helper to add an item to a drop down using indexes (and fixing the 0-index 
     * juce issue) 
     */
    void addItemToDropDown(
        const std::string & itemText,
        const int itemIndex,
        juce::ComboBox & dropDown);

    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    MidisynthesiserAudioProcessor & audioProcessor;

    // logger
    juce::Logger & logger;

    // params.  Only needed because the combobox is buggy and we need to manually
    // write id selections into params from the callback.
    std::shared_ptr<juce::AudioProcessorValueTreeState> pParams;

    // midi keyboard
    juce::MidiKeyboardComponent keyboardComponent;

    // gain control
    juce::Label gainLabel;
    juce::Slider gainSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttachment;

    juce::Label voiceSeparatorLabel;

    // WaveIndex slider
    // This allows partial selection which interpolates between different waves.
    juce::Label waveIndexLabel;
    juce::Slider waveIndexKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> waveIndexAttachment;

    // cutoff knob
    juce::Label cutoffLabel;
    juce::Slider cutoffKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> cutoffAttachment;

    // resonance knob
    juce::Label resonanceLabel;
    juce::Slider resonanceKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> resonanceAttachment;

    juce::Label effectsSeparatorLabel;

    // FX controls, one per effect
    struct Knob {
        juce::Label label;
        juce::Slider knob;
        std::shared_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> pAttachment = nullptr;
    };
    struct DropDown {
        //juce::Label label;
        juce::ComboBox dropDown;
        // NOTE the attachment class for comboboxes does not behave properly.
        // it stores the wrong values in the param (the combobox index) rather than
        // the item ID as expected.  This causes issues because 0 is invalid but
        // is expected to be a valid value.
        std::shared_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> pAttachment = nullptr;
    };
    struct FxControl {
        juce::Label headerLabel;
        DropDown typeSelector;
        Knob levelKnob;
    };
    std::deque<std::shared_ptr<FxControl>> fxControls;

    // helper to set the visibility of controls when non-null effect is selected
    void setFxGroupVisible(FxControl & fxControl, const bool visible = true);

    // Oscilloscope
    ScopeComponent<float> scopeComponent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidisynthesiserAudioProcessorEditor)
};
