/*
 ____  _____ _        _    
| __ )| ____| |      / \   
|  _ \|  _| | |     / _ \  
| |_) | |___| |___ / ___ \ 
|____/|_____|_____/_/   \_\

The platform for ultra-low latency audio and sensor processing

http://bela.io

A project of the Augmented Instruments Laboratory within the
Centre for Digital Music at Queen Mary University of London.
http://www.eecs.qmul.ac.uk/~andrewm

(c) 2016 Augmented Instruments Laboratory: Andrew McPherson,
    Astrid Bin, Liam Donovan, Christian Heinrichs, Robert Jack,
    Giulio Moro, Laurel Pardue, Victor Zappi. All rights reserved.

The Bela software is distributed under the GNU Lesser General Public License
(LGPL 3.0), available here: https://www.gnu.org/licenses/lgpl-3.0.txt
*/
/**
 * The code was referenced in https://forum.bela.io/d/1304-how-to-properly-stop-the-execution-of-an-auxiliary-task/.
 */

#include <Bela.h>
#include <libraries/Scope/Scope.h>
#include <cmath>
#include <fcntl.h>
#include <linux/input.h>

// set the frequency of the oscillators
float gFrequency = 110.0;
float gPhase;
float gInverseSampleRate;

int gKeyboardFd = -1;

// global variables to receive keys from the auxiliary thread
bool newKeyEvent = false;
int keyValue = 0;
int keyType = 0;

// instantiate the scope
Scope scope;

// Task for read the qwerty kbd
AuxiliaryTask gInputTask;

void read_kbd(void*);

bool setup(BelaContext *context, void *userData)
{
    // tell the scope how many channels and the sample rate
    scope.setup(2, context->audioSampleRate);

    gPhase = 0;
    gInverseSampleRate = 1.0f/context->audioSampleRate;

    // start qwerty kbd capture thread
    if((gInputTask = Bela_createAuxiliaryTask(&read_kbd, 50, "bela-read-input")) == 0)
    {
        return false;
    }

    return true;
}

void render(BelaContext *context, void *userData)
{
    static float out2 = 0;

    // iterate over the audio frames and create three oscillators, seperated in phase by PI/2
    for (unsigned int n = 0; n < context->audioFrames; ++n)
    {
        float out = 0.8f * sinf(gPhase);

        gPhase += 2.0f * (float)M_PI * gFrequency * gInverseSampleRate;
        if(gPhase > M_PI)
        {
            gPhase -= 2.0f * (float)M_PI;
        }

        // log the oscillator and the keyboard event to the scope
        scope.log(out, out2);
    }

    if(newKeyEvent)
    {
        if(keyType == 0)
        {
            out2 = 0.0f;
        }
        else if(keyType == 1)
        {
            out2 = 0.8f;
        }
        newKeyEvent = false;
    }

    Bela_scheduleAuxiliaryTask(gInputTask);

}

void cleanup(BelaContext *context, void *userData)
{
    if(gKeyboardFd)
    {
        close(gKeyboardFd);
    }
}

void read_kbd(void*)
{
    ssize_t num_read;
    struct input_event ev;
    struct timeval tv;
    fd_set readfds;
    const char *dev = "/dev/input/event1";

    gKeyboardFd = open(dev, O_RDONLY);
    if (gKeyboardFd == -1)
    {
        fprintf(stderr, "Cannot open file \"%s\".\n", dev);
        return;
    }

    while( !gShouldStop )
    {
        FD_ZERO(&readfds);
        FD_SET(gKeyboardFd, &readfds);
        tv.tv_sec = 0;
        tv.tv_usec = 1000;

        // Check if there are any characters ready to be read
        int num_readable = select(gKeyboardFd + 1, &readfds, NULL, NULL, &tv);

        if( num_readable > 0 )
        {
            num_read = read(gKeyboardFd, &ev, sizeof(ev) );
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

    return;
}
