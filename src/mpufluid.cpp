#ifdef __linux__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fluidsynth.h>
#include "SDL.h"

#include "common.h"
#include "musapi.h"
#include "fx.h"
#include "prefapi.h"

fluid_settings_t *settings;
fluid_synth_t *synth = NULL;
fluid_audio_driver_t *adriver = NULL;

void FS_DeInit(void);
/***************************************************************************
ALSA_Init() - 
 ***************************************************************************/
int FS_Init(int option) 
{
    printf("FS_Init\n");
    char fn[128];
    INI_GetPreference("Setup", "SoundFont", fn, 127, "SoundFont.sf2");

    /* Create the settings object. This example uses the default
     * values for the settings. */
    settings = new_fluid_settings();

    if(settings == NULL)
    {
        fprintf(stderr, "Failed to create the settings\n");
        FS_DeInit();
        return -1;
    }

    /* Create the synthesizer */
    synth = new_fluid_synth(settings);
 
    if(synth == NULL)
    {
        fprintf(stderr, "Failed to create the synthesizer\n");
        FS_DeInit();
        return -1;
    }
 
    /* Load the soundfont */
    if(fluid_synth_sfload(synth, fn, 1) == -1)
    {
        fprintf(stderr, "Failed to load the SoundFont\n");
        FS_DeInit();
        return -1;
    }
 
    fluid_settings_setstr(settings, "audio.driver", "sdl2");

    /* Create the audio driver. As soon as the audio driver is
     * created, the synthesizer can be played. */
    adriver = new_fluid_audio_driver(settings, synth);
 
    if(adriver == NULL)
    {
        fprintf(stderr, "Failed to create the audio driver\n");
        FS_DeInit();
        return -1;
    }
    return 1;
}

/***************************************************************************
ALSA_DeInit() -
 ***************************************************************************/
void FS_DeInit(void){

    if(adriver)
    {
        delete_fluid_audio_driver(adriver);
    }
 
    if(synth)
    {
        delete_fluid_synth(synth);
    }
 
    if(settings)
    {
        delete_fluid_settings(settings);
    }
 
    return;
}

/***************************************************************************
MPU_MapChannel() -
 ***************************************************************************/
static unsigned int 
MPU_MapChannel(
	unsigned chan
)
{
	if (chan < 9)
		return chan;
	
	if (chan == 15)
		return 9;
	
	return chan + 1;
}

/***************************************************************************
KeyOffEvent() -
 ***************************************************************************/
static void 
KeyOffEvent(
	unsigned int chan, 
	unsigned int key
)
{
	int err;
	
    fluid_synth_noteoff(synth, MPU_MapChannel(chan), key);
}

/***************************************************************************
KeyOnEvent() -
 ***************************************************************************/
static void 
KeyOnEvent(
	int chan, 
	unsigned int key, 
	unsigned int volume
) 
{
	int err;
	
    fluid_synth_noteon(synth, MPU_MapChannel(chan), key, volume);
}

/***************************************************************************
ProgramEvent() -
 ***************************************************************************/
static void 
ProgramEvent(
	unsigned int chan, 
	unsigned int param
) 
{
	int err;
	fluid_synth_program_change(synth, MPU_MapChannel(chan), param);
}

/***************************************************************************
PitchBendEvent() -
 ***************************************************************************/
static void 
PitchBendEvent(
	unsigned int chan, 
	int bend
) 
{
	int err;
	fluid_synth_pitch_bend(synth, MPU_MapChannel(chan), bend);
}

/***************************************************************************
AllNotesOffEvent() -
 ***************************************************************************/
static void 
AllNotesOffEvent(
	unsigned int chan, 
	unsigned int param
) 
{
	int err;
	fluid_synth_program_change(synth, chan, param);
}

/***************************************************************************
ControllerEvent() -
 ***************************************************************************/
static void 
ControllerEvent(
	unsigned int chan, 
	unsigned int controller, 
	unsigned int param
)
{
    static int event_map[] = {
	  0, 0, 1, 7, 10, 11, 91, 93, 64, 67, 120, 123, -1, -1, 121, -1
	};
	int err;

	fluid_synth_cc(synth, MPU_MapChannel(chan), event_map[controller], param);
}

musdevice_t mus_device_fs = {
	FS_Init,
	FS_DeInit,
	NULL,

    KeyOffEvent,
    KeyOnEvent,
    ControllerEvent,
    PitchBendEvent,
	ProgramEvent,
	AllNotesOffEvent,
};
#endif