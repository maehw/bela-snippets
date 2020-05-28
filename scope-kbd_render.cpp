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
 		return false;
    
	return true;
}

float lastOut = 0.0;
float lastOut2 = 0.0;

void render(BelaContext *context, void *userData)
{
	static float out4 = 0;
	// iterate over the audio frames and create three oscillators, seperated in phase by PI/2
	for (unsigned int n = 0; n < context->audioFrames; ++n)
	{
		float out = 0.8f * sinf(gPhase);
		float out2 = 0.8f * sinf(gPhase - (float)M_PI/2.f);

		gPhase += 2.0f * (float)M_PI * gFrequency * gInverseSampleRate;
		if(gPhase > M_PI)
			gPhase -= 2.0f * (float)M_PI;
			
		// log the three oscillators to the scope
		scope.log(out, out4);
        
		// optional - tell the scope to trigger when oscillator 1 becomes less than oscillator 2
		// note this has no effect unless trigger mode is set to custom in the scope UI
		if (lastOut >= lastOut2 && out < out2)
		{
			scope.trigger();
		}

		lastOut = out;
		lastOut2 = out2;
	}
	
	if(newKeyEvent)
	{
		if(keyType == 0)
		{
			out4 = 0.0f;
		}
		else if(keyType == 1)
		{
			out4 = 0.8f;
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
	// Variables keyboard control
	struct input_event ev;
	ssize_t n;
	const char *dev = "/dev/input/event1";
	
	// qwerty keyboard capture
	gKeyboardFd = open(dev, O_RDONLY);
	if (gKeyboardFd == -1) {
		fprintf(stderr, "Cannot open %s:.\n", dev);
		return;
	}
	
	for(;;)
	{
		n = read(gKeyboardFd, &ev, sizeof ev);
		if (sizeof ev == n)
		{
	        if (ev.type == EV_KEY && ev.value == 0)
	        {
				keyType = 0; // key released
	         	keyValue = (int)ev.code;
	         	newKeyEvent = true;
	        }
			else if (ev.type == EV_KEY && ev.value == 1)
			{
				keyType = 1; // key pressed
				keyValue = (int)ev.code;
				newKeyEvent = true;
			}
	    }
	}
	
	return;
}