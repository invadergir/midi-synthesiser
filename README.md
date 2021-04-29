# Midi Synthesiser Starter Code

This repo is the result of my research into the Juce Framework and is a good starting point to create a synthesiser app and/or plugin.  It contains some good reusable generic classes that can be used to build any kind of midi synthesiser  with effects.

This code depends on the Juce Library to run.  Please see the [LICENSE.md](LICENSE.md) and [LICENSE-NOTE.md](LICENSE-NOTE.md) files for details.

--------------------------------------------------

# A Word Of Warning Before You Start:  Turn Down Your Volume!

**This synthesiser lets you put as many as 6 effects on the signal, in any order you like, in any number you like.  This means you could place 6 distortions in-line or 6 reverbs if you really wanted to....  However - you probably DO NOT want to, as feedback, frequency spikes and general chaos will be the result.  It's fun to play with, but it gets out of hand quickly - especially if your effect gain dials aren't set low enough.**

* _**So... before loading up on more than one effect of the same type, MAKE SURE TO TURN DOWN YOUR VOLUME!**_

**The filter resonance can also be especially loud, even at low settings.**

**So in case you missed it, I'll say it again:  TURN DOWN YOUR VOLUME!**  

_**You've been warned.**_  **I'm not liable for any damage to your speakers, ears, sanity, etc.**

--------------------------------------------------

## Musical Features

Cool stuff in here:

1. A wavetable synthesiser implementation with waveform sweeping from sine to triangle to square, and any combination between each.  (You can sweep continuously between sine and triangle and between triangle and square.)
2. An effects section featuring six effect slots with variable gain that can be put in any order you like.  (You might have heard about this above, in the Warning section...)  The effects use juce::dsp modules.  The available effects are:
    * Distortion - mild wave distortion taken from an overdriven filter circuit
    * Delay - ~400ms delay.  Feedback (level of each repeat) is controlled by the effect level control.
    * Chorus - Wet / dry mix is controlled by the effect level (1.0 = full wet)
    * Reverb - Wet / dry mix is controlled by the effect level control.
3. A low-pass filter with configurable cutoff frequency and resonance.  (Pretty standard stuff, but please note that the filter goes into steep resonance pretty early on.  The default resonance is 0.)
    * (Have I mentioned that it's a good idea to turn down the volume before testing this synth??)

## Software Features

The project is a good starting point for any type of synthesiser project, as well as a proving ground for my method of creating a flexible effects chain that is dynamically malleable at runtime.  Essential to the design is my "*Single-Precision Decision*", where I decided to only support audio processing on "`float`" types, which allowed me to create interfaces and encapsulate the behavior of effect processors and synths more flexibly (and allow for the mix & match effect section).  This came at the cost of not having double-precision support; to add double-precision support, it will take a little bit of doing but is very possible.  (Please see Planned feature #1, below, or [here](https://github.com/invadergir/juce-double-precision-poc) for more details.)

Some of the main classes and interfaces are described here.  A UML class diagram is located at [doc/class-diagram.png](doc/class-diagram.png).  The generic, reusable code is stored in the "`Source/juce_igutil`" subdirectory.  At some point this will be pulled out into a proper Juce module.

1. *SynthAudioSource* - an AudioSource - like interface for things that create audio and expect midi input.
    1. ConfigurableSynthAudioSource - an implementation of SynthAudioSource that provides facility to configure it with any Synthesiser, SynthesiserVoice, SynthesiserSound, and Processor (effect) types.  The design is meant to allow strategies for all those types to be plugged in and new implementations for those strategies easily created.  The SynthesiserVoice and SynthesiserSound classes are Juce abstract classes, but the idea is the same.
    2. WavetableSynth - an implementation of SynthAudioSource that decorates a ConfigurableSynthAudioSource.  Uses WavetableSynthVoice/WavetableOscillator as well as UnlimitedSynthSound.
2. *Oscillator* - generic Oscillator interface for synthesisers.
    1. WavetableOscillator - implements Oscillator and provides a way to iterate through any provided wavetables to simulate a waveform generator.  Completely configurable to allow new wavetables.
3. *Processor* - generic effect processor interface.
    1. EffectProcessor - template processor class implements Processor and is meant to wrap "`juce::dsp`" classes.
    2. ProcessorSequence - similar to "`juce::dsp::ProcessorChain`" but for my Processors.  Much more flexible than ProcessorChain because it uses runtime polymorphism rather than hardcoded compile-time typing.
    3. DelayProcessor - effect that uses a juce DelayLine to produce a standard delay effect.
4. WavetableSynthVoice - juce::SynthesiserVoice implementation / extension that uses WavetableOscillator(s).

The malleable effects processing sequence was realized by the ProcessorSequence and some fancy footwork inside the WavetableSynth.  At startup, all of the possible effects are created (to avoid processing delays when rendering).  They are added to a pool of effects and then pulled out into a ProcessorSequence when selected in the UI.  This effects section could be pulled out into a generic module probably.

To allow for control of gain parameters in each effect, a UI knob is created for each effect slot 1-6 with a corresponding Parameter.  The parameters are read at the start of each render cycle and set appropriately through lambda functions attached to the effect.  To get more detailed effect parameters, there needs to be fancier UI handling (see Planned Features below).

## How to Build

1. Install Juce:  [https://juce.com/](https://juce.com/)
    * Note: Juce is a cross-platform framework that should work fine on Windows, Mac, and Linux; however I've only tested it on Windows.  If you use a Mac (or you are a brave soul who does audio on Linux), please let me know if there are any incompatabilities.
1. Install an appropriate C++ development environment to compile C++ projects.  (For example, on Windows you'll need MS Visual Studio 2019)
2. Open the "`midi-synthesiser.jucer`" file in the Projucer.
3. Change the global path to the Juce modules to match your system.  By default it is set to "`../../../../../opt/juce/modules`" which has very low odds of matching your local setup.
4. Save the jucer project, then open the C++ project in your chosen IDE.
5. Build the project. (In Visual Studio, select "`Build / Rebuild Solution`".)

## How to Run

* Standalone, after building in the IDE (easiest way):
    * In the IDE (MS Visual Studio), click on the run button labelled "Local Windows Debugger" to run it in standalone mode, or double-click the EXE that is produced.
* As a plugin, inside an audio host program:
    * Copy the *.vst3 file to the appropriate location and load it in your host.  The VST is generated in `"./Builds/VisualStudio2019/x64/Debug/VST3/midi-synthesiser.vst3`", and on Windows systems you should copy it to "`C:\Program Files\Common Files\VST3\`"

## Planned Features

New features that I plan on working on in my spare time include:

1. Double-precision support, to be gained from the conversion to the build process for auto-generating double-precision code, a la [https://github.com/invadergir/juce-double-precision-poc](https://github.com/invadergir/juce-double-precision-poc).
2. Swappable "deep" controls for the effects, where a UI Component with lots of attached knobs is created/destroyed properly when a new effect is chosen.  (For example, delay time, number of repeats, etc.)  This requires figuring out a better way to handle the Params for each control without using too much memory.
3. Pull out "`juce_igutil`" code into a separate repo containing a proper juce module or library.  
4. Extract malleable effects processing section from WavetableSynth into reusable module.
5. Profile memory usage and possibly / probably limit the number of duplicate effects to 2 or 3.  Creating 6 effect instances of each type uses a bit of memory, especially the delays.
99. Resolve various TODOs.

## Known Issues

1. Multiple instances of the same effect can be very loud. In case you haven't heard yet, you probably want to TURN DOWN YOUR VOLUME if you start using extreme effects settings or multiple effect instances of the same type.....
2. The standalone audio device detection and usage is straight from Juce, and as such, it is not perfectly robust.  On my dev system, I can only get driver buffer sizes of 144 samples but in Studio One it goes with what is on the host, and that goes as low as your hardware driver allows.
