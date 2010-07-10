#include "sfx.h"

//---------------------------------------------------------------------------------
/* Sound effects */
//---------------------------------------------------------------------------------

void snd_fx_tick()
{
#ifdef USE_MODPLAYER
	ASND_SetVoice(2, VOICE_MONO_8BIT, 4096*6*2,0, &sound_effects[0][0], 2048/128, 64, 64, NULL);
#endif
}
void snd_fx_yes()
{
#ifdef USE_MODPLAYER
	ASND_SetVoice(1, VOICE_MONO_8BIT, 12000,0, &sound_effects[1][0], 2048/4, 63, 63, NULL);
#endif
}

void snd_fx_no()
{
#ifdef USE_MODPLAYER
	ASND_SetVoice(1, VOICE_MONO_8BIT, 4096,0, &sound_effects[0][0], 2048/16, 96, 96, NULL);
#endif
}

void snd_fx_scroll()
{
#ifdef USE_MODPLAYER
	ASND_SetVoice(2, VOICE_MONO_8BIT, 1024,0, &sound_effects[0][0], 64, 64, 64, NULL);
#endif
}

void snd_fx_fx(int percent)
{
#ifdef USE_MODPLAYER
	ASND_SetVoice(2, VOICE_MONO_8BIT, 5000+percent*250,0, &sound_effects[0][0], 128, 64, 64, NULL);
#endif
}
