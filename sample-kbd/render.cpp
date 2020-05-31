/**
 * This snippet demonstrates the use of a keyboard plugged into Bela directly.
 * This has been used to check if Bela works together MaKey MaKey.
 * As soon as a key (any key) is pushed down, a sound file will be played back.
 * Hint: You'll need the additional "waves.wav" file from the Bela examples.
 */

#include <Bela.h>
#include <libraries/AudioFile/AudioFile.h>
#include <vector>
#include <fcntl.h>
#include <linux/input.h>

// global variables to receive keys from the auxiliary thread
bool newKeyEvent = false;
int keyValue = 0;
int keyType = 0;

std::string gFilename = "waves.wav";
const char *gKeyboardDev = "/dev/input/event1";

int gStartFrame = 44100;
int gEndFrame = 88200;

std::vector<std::vector<float> > gSampleData;

int gReadPtr;   // Position of last read sample from file

// Task for read the qwerty kbd
AuxiliaryTask gInputTask;

void read_kbd(void*);

bool setup(BelaContext *context, void *userData)
{
    gSampleData = AudioFileUtilities::load(gFilename, gStartFrame, gEndFrame - gStartFrame);
    
    // start qwerty kbd capture thread
    if((gInputTask = Bela_createAuxiliaryTask(&read_kbd, 50, "bela-read-input")) == 0)
    {
        return false;
    }

    return true;
}

void render(BelaContext *context, void *userData)
{
    static bool bTriggered = false;
    static bool bFinished = false;

    for(unsigned int n = 0; n < context->audioFrames; n++) {
        
        if( bTriggered )
        {
            /* if triggered then increment read pointer and stop when end of file is reached */
            ++gReadPtr;
            if( gReadPtr > gSampleData[0].size() )
            {
                bTriggered = false;
                bFinished = true;
            }
        }

        for(unsigned int channel = 0; channel < context->audioOutChannels; channel++)
        {
            float out = 0.0f; /* initialize with "silence" */

            /* only play sample when triggered and not finished yet */
            if( bTriggered && !bFinished )
            {
                /* wrap channel index in case there are more audio output channels than the file contains */
                out = gSampleData[channel%gSampleData.size()][gReadPtr];
            }

            audioWrite(context, n, channel, out);
        }
    }
    
    if(newKeyEvent)
    {
        /* start playing the audio file on key down event */
        if(keyType == 1)
        {
            gReadPtr = 0; /* play from the beginning */
            bTriggered = true;
            bFinished = false;
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
