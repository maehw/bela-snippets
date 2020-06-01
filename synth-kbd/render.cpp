/**
 * This snippet is based on the "additive-synth" project of
 * "C++ Real-Time Audio Programming with Bela - Lecture 5: Classes and Objects".
 * It uses additive synthesiser based on an array of Wavetable oscillator objects.
 * MaKey MaKey (or a keyboard) is used as input.
 */

#include <Bela.h>
#include <libraries/Gui/Gui.h>
#include <libraries/GuiController/GuiController.h>
#include <libraries/Scope/Scope.h>
#include <cmath>
#include <vector>
#include <sstream>
#include <fcntl.h>
#include <linux/input.h>

#include "wavetable.h"  // This is needed for the Wavetable class

// global variables to receive keys from the auxiliary thread
bool newKeyEvent = false;
int keyValue = 0;
int keyType = 0;


// Constants that define the program behaviour
const unsigned int kWavetableSize = 512;
const unsigned int kNumOscillators = 6;
const char *gKeyboardDev = "/dev/input/event1";

// Browser-based GUI to adjust parameters
Gui gGui;
GuiController gGuiController;

// Browser-based oscilloscope
Scope gScope;

// Oscillators for the additive synths
std::vector<Wavetable> gOscillators;
std::vector<float> gAmplitudes;
std::vector<int> gKeyCodes;

// Task for read the qwerty kbd
AuxiliaryTask gInputTask;

void read_kbd(void*);

bool setup(BelaContext *context, void *userData)
{
    std::vector<float> wavetable;
        
    // Populate a buffer with a sine wave
    wavetable.resize(kWavetableSize);
    for(unsigned int n = 0; n < wavetable.size(); n++) {
        wavetable[n] = sinf(2.0 * M_PI * (float)n / (float)wavetable.size());
    }
    
    // Create an array of oscillators
    for(unsigned int n = 0; n < kNumOscillators; n++) {
        Wavetable oscillator(context->audioSampleRate, wavetable, false);
        gOscillators.push_back(oscillator);
    }
    
    // Prepare the amplitude buffer
    gAmplitudes.resize(kNumOscillators);
    
    // Prepare the key code buffer
    gKeyCodes.resize(kNumOscillators);
    
    // Set up the GUI
    gGui.setup(context->projectName);
    gGuiController.setup(&gGui, "Wavetable Controller");    
    
    // Arguments: name, default value, minimum, maximum, increment
    gGuiController.addSlider("Amplitude (dB)", -30, -40, 0, 0);
    
    // Add MIDI note sliders for the individual oscillators (and initialize them to specific notes)
    gGuiController.addSlider("MIDI note #1", 60, 48, 84, 1);
    gGuiController.addSlider("MIDI note #2", 62, 48, 84, 1);
    gGuiController.addSlider("MIDI note #3", 64, 48, 84, 1);
    gGuiController.addSlider("MIDI note #4", 65, 48, 84, 1);
    gGuiController.addSlider("MIDI note #5", 67, 48, 84, 1);
    gGuiController.addSlider("MIDI note #6", 69, 48, 84, 1);

    // Set up the oscilloscope
    gScope.setup(1, context->audioSampleRate);
    
    // Mute all waves
    for(unsigned int i = 0; i < gOscillators.size(); i++)
    {
        gAmplitudes[i] = 0.0f;
    }
    
    // Initialize key map
    gKeyCodes[0] = KEY_A;
    gKeyCodes[1] = KEY_B;
    gKeyCodes[2] = KEY_C;
    gKeyCodes[3] = KEY_D;
    gKeyCodes[4] = KEY_E;
    gKeyCodes[5] = KEY_F;

    // start qwerty kbd capture thread
    if((gInputTask = Bela_createAuxiliaryTask(&read_kbd, 50, "bela-read-input")) == 0)
    {
        return false;
    }
    
    return true;
}

void render(BelaContext *context, void *userData)
{
    float amplitudeDB = gGuiController.getSliderValue(0); // Amplitude is first slider  
    float amplitude = powf(10.0, amplitudeDB / 20); // Convert dB to linear amplitude
    
    for(unsigned int i = 0; i < gOscillators.size(); i++)
    {
        float midiNote = gGuiController.getSliderValue(i+1); // MIDI note from individual slider
        float frequency = 440.0 * powf(2.0, (midiNote - 69.0) / 12.0);  // MIDI to frequency

        gOscillators[i].setFrequency(frequency);
    }
    
    for(unsigned int n = 0; n < context->audioFrames; n++) {
        float out = 0;
        
        for(unsigned int i = 0; i < gOscillators.size(); i++)
        {
            if( gAmplitudes[i] > 0.0f )
            {
                out += gAmplitudes[i] * gOscillators[i].process();
            }
        }
        
        // Scale global amplitude
        out *= amplitude;
            
        for(unsigned int channel = 0; channel < context->audioOutChannels; channel++) {
            // Write the sample to every audio output channel
            audioWrite(context, n, channel, out);
        }
        
        // Write the output to the oscilloscope
        gScope.log(out);
    }
    
    if(newKeyEvent)
    {
        for(unsigned int i = 0; i < gOscillators.size(); i++)
        {
            if( gKeyCodes[i] == keyValue )
            {
                if(keyType == 0)
                {
                    gAmplitudes[i] = 0.0f;
                }
                else if(keyType == 1)
                {
                    gAmplitudes[i] = 1.0f;
                }
            }
        }
        newKeyEvent = false;
    }

    Bela_scheduleAuxiliaryTask(gInputTask);
}

void cleanup(BelaContext *context, void *userData)
{
    
}

void read_kbd(void*)
{
    ssize_t num_read;
    struct input_event ev;
    struct timeval tv;
    int keyboardFd = -1;
    fd_set readfds;

    keyboardFd = open(gKeyboardDev, O_RDONLY);
    if (keyboardFd == -1)
    {
        fprintf(stderr, "Cannot open file \"%s\".\n", gKeyboardDev);
        return;
    }

    while( !gShouldStop )
    {
        FD_ZERO(&readfds);
        FD_SET(keyboardFd, &readfds);
        tv.tv_sec = 0;
        tv.tv_usec = 1000;

        // Check if there are any characters ready to be read
        int num_readable = select(keyboardFd + 1, &readfds, NULL, NULL, &tv);

        if( num_readable > 0 )
        {
            num_read = read(keyboardFd, &ev, sizeof(ev) );
            if( sizeof(ev) == num_read )
            {
                if(ev.type == EV_KEY && ev.value == 0)
                {
                    // key released
                    keyType = 0;
                    keyValue = (int)ev.code;
                    newKeyEvent = true;
                }
                else if(ev.type == EV_KEY && ev.value == 1)
                {
                    // key pressed
                    keyType = 1;
                    keyValue = (int)ev.code;
                    newKeyEvent = true;
                }
            }
        }
    }

    if(keyboardFd)
    {
        close(keyboardFd);
    }

    return;
}

