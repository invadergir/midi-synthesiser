/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

#include "Config.h"
#include "juce_igutil/ConfigurableSynthAudioSource.h"

using namespace std;
using namespace juce;
using namespace juce_igutil;
using namespace config;

// window size
const int windowW = 604;
const int windowH = 600;

// margin size (x & y buffer)
const int windowMargin = 7;

// "usable" window size (includes margin)
const int uWindowW = windowW - 2*windowMargin;
const int uWindowH = windowH - 2*windowMargin;

// keyboard
const 
const int keyboardX = windowMargin;
const int keyboardY = windowMargin;
const int keyboardW = uWindowW;
const int keyboardH = 75;

// Generic text label height & width
const int labelH = 17;
const int labelW = 80;

const int horizSliderW   = 300;

const int gainControlX = windowMargin;
const int gainControlY = keyboardY + keyboardH + 20; 

const int voiceSeparatorX = windowMargin;
const int voiceSeparatorY = gainControlY + labelH + 20;
const int separatorW = uWindowW;
const int separatorH = labelH;

const int knobW = 80;
const int knobH = 80;
const int knobTextBoxW = knobW - 2*windowMargin;
const int knobTextBoxH = labelH;
const int knobMargin = 20;
const int knobSepX = 10;
const int knobBigSepX = knobSepX *4;

const Rectangle<int> waveIndexKnobR(windowMargin, voiceSeparatorY + labelH + 20, knobW, knobH);

const Rectangle<int> cutoffKnobR(waveIndexKnobR.getTopRight().getX() + knobBigSepX, waveIndexKnobR.getY(), knobW, knobH);
const Rectangle<int> resonanceKnobRect(cutoffKnobR.getTopRight().getX() + knobSepX, cutoffKnobR.getY(), knobW, knobH);

const Rectangle<int> effectsSepR(waveIndexKnobR.getX(), resonanceKnobRect.getBottom() + labelH * 3, separatorW, separatorH);

const int perEffectOriginX = windowMargin;
const int perEffectOriginY = effectsSepR.getBottom() + labelH;

const int scopeX = windowMargin;
const int scopeY = perEffectOriginY + waveIndexKnobR.getHeight() + 50 + labelH;
const int scopeW = uWindowW;
const int scopeH = 250;

// Look and feel objects
LookAndFeel_V4 sliderLAF;
LookAndFeel_V3 knobLAF; // I think these are nicer than the latest knobs

#define LOG (logger.writeToLog)

//==============================================================================
MidisynthesiserAudioProcessorEditor::MidisynthesiserAudioProcessorEditor(
    MidisynthesiserAudioProcessor & p,
    std::shared_ptr<juce::AudioProcessorValueTreeState> pSynthParams
): 
    AudioProcessorEditor(&p), 
    audioProcessor(p),
    logger(*Logger::getCurrentLogger()),
    pParams(pSynthParams),
    keyboardComponent(p.getKeyboardState(), juce::MidiKeyboardComponent::horizontalKeyboard),
    scopeComponent(p.getAudioBufferQueue())
{
    LOG(String("PluginEditor constructor.  maxEffects = ") + String(config::maxEffects));

    addAndMakeVisible(keyboardComponent);
    
    // See timerCallback() - it initializes keyboard focus to be on the keyboard component
    startTimer(400);

    // gain slider
    gainLabel.setText("Master Gain", juce::dontSendNotification);
    //gainSlider.setLookAndFeel(&sliderLAF); // normal LAF is fine
    addAndMakeVisible(gainLabel);
    addAndMakeVisible(gainSlider);
    gainAttachment.reset( 
        new AudioProcessorValueTreeState::SliderAttachment( 
            *pParams, gainPN, gainSlider ) );

    voiceSeparatorLabel.setText("------------------------------------------------------ VOICE CONTROL ------------------------------------------------------", dontSendNotification);
    addAndMakeVisible(voiceSeparatorLabel);

    // WaveIndex knob
    waveIndexLabel.setText("Wave Select", juce::dontSendNotification);
    setKnobStyle(waveIndexKnob);
    addAndMakeVisible(waveIndexLabel);
    addAndMakeVisible(waveIndexKnob);
    waveIndexAttachment.reset( 
        new AudioProcessorValueTreeState::SliderAttachment( 
            *pParams, waveIndexPN, waveIndexKnob ) );

    // Cutoff slider
    cutoffLabel.setText("Filter Cutoff (Hz)", juce::dontSendNotification);
    setKnobStyle(cutoffKnob);
    addAndMakeVisible(cutoffLabel);
    addAndMakeVisible(cutoffKnob);
    cutoffAttachment.reset( 
        new AudioProcessorValueTreeState::SliderAttachment( 
            *pParams, cutoffPN, cutoffKnob ) );

    // Resonance slider
    resonanceLabel.setText("Filter Resonance", juce::dontSendNotification);
    setKnobStyle(resonanceKnob);
    addAndMakeVisible(resonanceLabel);
    addAndMakeVisible(resonanceKnob);
    resonanceAttachment.reset( 
        new AudioProcessorValueTreeState::SliderAttachment( 
            *pParams, resonancePN, resonanceKnob ) );

    effectsSeparatorLabel.setText("--------------------------------------------------------- EFFECTS ---------------------------------------------------------", dontSendNotification);

    addAndMakeVisible(effectsSeparatorLabel);

    // controls for each effect
    for ( int ix=0; ix<maxEffects; ++ix ) {
        fxControls.push_back( make_shared<FxControl>() );

        const int displayNum = ix + 1;
        auto & headerLabel = fxControls.back()->headerLabel;
        headerLabel.setText(String("FX Slot #")+String(displayNum), dontSendNotification);
        // none of these havea an effect (why?):
        //headerLabel.getFont().setStyleFlags(juce::Font::underlined);
        //headerLabel.getFont().setUnderline(true);
        //headerLabel.getFont().setHeight(20.0);
        addAndMakeVisible(headerLabel);

        // effect type dropdown/combobox
        DropDown & typeDD = fxControls.back()->typeSelector;
        addItemToDropDown("(None)",     NULL_EFFECT,       typeDD.dropDown);
        addItemToDropDown("Distortion", DISTORTION_EFFECT, typeDD.dropDown);
        addItemToDropDown("Chorus",     CHORUS_EFFECT,     typeDD.dropDown);
        addItemToDropDown("Delay",      DELAY_EFFECT,      typeDD.dropDown);
        addItemToDropDown("Reverb",     REVERB_EFFECT,     typeDD.dropDown);
        typeDD.dropDown.onChange = [this, ix] { this->changedEffectType(ix); };
        addAndMakeVisible(typeDD.dropDown);
        typeDD.pAttachment.reset( 
            new AudioProcessorValueTreeState::ComboBoxAttachment( 
                *pParams, getEffectPN(typeSelectorPN, ix), typeDD.dropDown));

        // This is too much text; I don't think this is needed.  
        // Save... Maybe later if we can separate the main header from this via 
        // fonts/underlining we can reuse this.
        //typeDD.label.setText(String("FX #") + String(displayNum) + String(" Type"), dontSendNotification);
        //addAndMakeVisible(typeDD.label);

        // generic level/gain control for the effect
        Knob & levelKnob = fxControls.back()->levelKnob;
        String labelText = String("FX #") + String(displayNum) + " Level";
        levelKnob.label.setText(labelText, juce::dontSendNotification);
        addAndMakeVisible(levelKnob.label); 
        setKnobStyle(levelKnob.knob);
        addChildComponent(levelKnob.knob);
        levelKnob.pAttachment.reset( 
            new AudioProcessorValueTreeState::SliderAttachment( 
                *pParams, getEffectPN(fxLevelPN, ix), levelKnob.knob ) );
    }

    //addAndMakeVisible(scopeComponent);

    // Do this last
    setSize(windowW, windowH);
}

/**
 * destructor
 */
MidisynthesiserAudioProcessorEditor::~MidisynthesiserAudioProcessorEditor()
{
    //delete testDD;
}

/**
 * Helper to add an item to a drop down using indexes (and fixing the 0-index 
 * juce issue) 
 */
void MidisynthesiserAudioProcessorEditor::addItemToDropDown(
    const std::string & itemText,
    const int itemIndex,
    juce::ComboBox & dropDown)
{
    jassert(itemIndex >= 0 && "index can't be negative or that may allow a 0 index when we add 1");
    dropDown.addItem(itemText, itemIndex+1);
}


/**
 * Callback that happens when the effect type changes.
 * 
 * @param index of the effect [0-maxEffects)
 */
void MidisynthesiserAudioProcessorEditor::changedEffectType(const int fxGroupIndex)
{
    // get the new selected id
    const int fxType = fxControls[fxGroupIndex]->typeSelector.dropDown.getSelectedItemIndex();
    LOG(String("Effect Type Combobox #") + String(fxGroupIndex) << " just changed.  New value is: " << fxType);

    // adjust the UI
    if (fxType >= FIRST_REAL_EFFECT ) {
        setFxGroupVisible(*(fxControls[fxGroupIndex]), true);
    }
    else {
        setFxGroupVisible(*(fxControls[fxGroupIndex]), false);
    }
}

//==============================================================================
void MidisynthesiserAudioProcessorEditor::paint( juce::Graphics & g )
{
    // add a nice background color (RGB; dark green)
    g.fillAll( Colour(0x0, 0x0F, 0x0) );
}

/** 
 * Set the component layout. 
 */
void MidisynthesiserAudioProcessorEditor::resized()
{
    keyboardComponent.setBounds(keyboardX, keyboardY, keyboardW, keyboardH);

    gainLabel.setBounds(gainControlX, gainControlY, labelW, labelH);
    gainSlider.setBounds(gainControlX+labelW, gainControlY, horizSliderW, labelH);

    voiceSeparatorLabel.setBounds(voiceSeparatorX, voiceSeparatorY, separatorW, separatorH);

    waveIndexKnob.setBounds(waveIndexKnobR);
    alignLabelToKnob(waveIndexLabel, waveIndexKnob);

    cutoffKnob.setBounds(cutoffKnobR); 
    alignLabelToKnob(cutoffLabel, cutoffKnob, 2);

    //resonanceKnob.setBounds(resonanceKnobX, resonanceKnobY, knobW, knobH);
    resonanceKnob.setBounds(resonanceKnobRect);
    alignLabelToKnob(resonanceLabel, resonanceKnob, 2);

    effectsSeparatorLabel.setBounds(effectsSepR);

    // per-effect controls 
    for (int ix = 0; ix < fxControls.size(); ++ix ) {
        FxControl & fxGroup = *(fxControls[ix]);
        const int colX = perEffectOriginX + ix*(knobW + knobMargin);
        const Rectangle<int> headerLabelR(colX, perEffectOriginY, knobW, labelH*2);
        fxGroup.headerLabel.setBounds(headerLabelR);

        //const Rectangle<int> ddLabelR(colX, headerLabelR.getBottom() + labelH, knobW, labelH);
        //const Rectangle<int> ddLabelR(colX, perEffectOriginY, knobW, labelH);
        //fxGroup.typeSelector.label.setBounds(ddLabelR);

        const Rectangle<int> dropDownR(colX, headerLabelR.getBottom() + 7, knobW+10, labelH);
        fxGroup.typeSelector.dropDown.setBounds(dropDownR);

        fxGroup.levelKnob.knob.setBounds(colX, dropDownR.getBottom() + labelH, knobW, knobH);
        alignLabelToKnob(fxGroup.levelKnob.label, fxGroup.levelKnob.knob);

        // todo merge with code in changedEffectType(), create fn
        if (fxGroup.typeSelector.dropDown.getSelectedItemIndex() >= FIRST_REAL_EFFECT) {
            setFxGroupVisible(fxGroup, true);
        }
        else {
            setFxGroupVisible(fxGroup, false);
        }
    }

    scopeComponent.setTopLeftPosition(scopeX, scopeY);
    scopeComponent.setSize(scopeW, scopeH);
}

/**
 * Helper to set the knob style the same for every control knob
 */
void MidisynthesiserAudioProcessorEditor::setKnobStyle(juce::Slider & knob)
{
    knob.setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
    knob.setTextBoxStyle(Slider::TextBoxAbove, false, knobTextBoxW, knobTextBoxH);
    knob.setLookAndFeel(&knobLAF);
    const Colour knobColour(Colour(0, 0x0F0, 0)); // rgb
    knob.setColour(Slider::ColourIds::thumbColourId, knobColour);
    knob.setColour(Slider::ColourIds::rotarySliderFillColourId, knobColour);
}

// Helper to align a label to a knob in a standard way
void MidisynthesiserAudioProcessorEditor::alignLabelToKnob(
    juce::Label & label, 
    juce::Slider & knob, 
    const int numLines)
{
    const int knobH = numLines * labelH;
    label.setBounds(knob.getX(), knob.getBounds().getBottom(), knobW, knobH);
}

// helper to set the visibility of controls when non-null effect is selected
void MidisynthesiserAudioProcessorEditor::setFxGroupVisible(
    FxControl & fxControl, 
    const bool visible)
{
    fxControl.levelKnob.label.setVisible(visible);
    fxControl.levelKnob.knob.setVisible(visible);
}

