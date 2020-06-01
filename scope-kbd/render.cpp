/**
 * This snippet demonstrates the use of a keyboard plugged into Bela directly.
 * Have a look at the Bela Oscilloscope and then type on your keyboard.
 * Any key stroke will show as a positive level. 
 * This has been used to check if Bela works together MaKey MaKey.
 * The code was referenced in https://forum.bela.io/d/1304-how-to-properly-stop-the-execution-of-an-auxiliary-task/.
 */

#include <Bela.h>
#include <libraries/Scope/Scope.h>
#include <cmath>
#include <fcntl.h>
#include <linux/input.h>

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
    scope.setup(1, context->audioSampleRate);

    // start qwerty kbd capture thread
    if((gInputTask = Bela_createAuxiliaryTask(&read_kbd, 50, "bela-read-input")) == 0)
    {
        return false;
    }

    return true;
}

void render(BelaContext *context, void *userData)
{
    static float out = 0.0f;

    // iterate over the audio frames and create three oscillators, seperated in phase by PI/2
    for (unsigned int n = 0; n < context->audioFrames; ++n)
    {
        // log the oscillator and the keyboard event to the scope
        scope.log(out);
    }

    if(newKeyEvent)
    {
        if(keyType == 0)
        {
            out = 0.0f;
        }
        else if(keyType == 1)
        {
            out = 1.0f;
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
    const char *dev = "/dev/input/event1";

    keyboardFd = open(dev, O_RDONLY);
    if (keyboardFd == -1)
    {
        fprintf(stderr, "Cannot open file \"%s\".\n", dev);
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
