#include <Bela.h>
#include <Scope/Scope.h>
#include <Keys.h>

Scope scope;

#define SCANNER
//#define LOG_KEYS_AUDIO
#define FEATURE

//#define PIEZOS
#define LOWPASS_PIEZOS
#define DELAY_INPUTS

#ifdef SCANNER
#include <Keys.h>
Keys* keys;
BoardsTopology bt;
int gKeyOffset = 59;
#ifdef FEATURE
#include "KeyFeature.h"
KeyFeature keyFeature;
#endif /* FEATURE */
#else /* SCANNER */
#undef LOG_KEYS_AUDIO
#undef FEATURE
#endif /* SCANNER */

static const unsigned int gNumPiezos = 8; // let's keep this even if no piezos
#ifdef PIEZOS
unsigned int gMicPin[gNumPiezos] = {7, 6, 5, 4, 3, 2, 1, 0};
#ifdef LOWPASS_PIEZOS
#include <IirFilter.h>
std::vector<IirFilter> piezoFilters;
std::array<std::array<double, IIR_FILTER_STAGE_COEFFICIENTS>, 2> piezoFiltersCoefficients 
{{
	//[B, A] = butter(2, 500/22050); fprintf('%.8f, ', [B A(2:3)]); fprintf('\n')
	{{ 0.00120741, 0.00241481, 0.00120741, -1.89933342, 0.90416304 }},
	{{1, 0, 0, 0, 0}},
}};
#endif /* LOWPASS_PIEZOS */

unsigned int gInputDelayIdx = 0;
float gInputDelayTime = 0.0067;
unsigned int gInputDelaySize;
std::vector<std::vector<float>> gInputDelayBuffers;

#else /* PIEZOS */
#undef LOWPASS_PIEZOS
#undef DELAY_INPUTS
#endif /* PIEZOS */

#ifdef SCANNER
void Bela_userSettings(BelaInitSettings* settings)
{
	settings->pruNumber = 0;	
	settings->useDigital = 0;
}
#ifndef LOG_KEYS_AUDIO
void postCallback(void* arg, float* buffer, unsigned int length){
	Keys* keys = (Keys*)arg;
#ifdef FEATURE
	KeyFeature::postCallback((void*)&keyFeature, buffer, length);
#endif /* FEATURE */
	
	static float oldVal;
	static float oldVal2;
	unsigned int key = gKeyOffset + 1;
	unsigned int key2 = gKeyOffset + 2;
	float val = keys->getNoteValue(key);
	float val2 = keys->getNoteValue(key2);
#ifdef FEATURE
	float feature = keyFeature.percussiveness[key].getFeature();
	float feature2 = keyFeature.percussiveness[key2].getFeature();
	scope.log(val, (val - oldVal), feature, 0, 0, 0, 0, 0);
#endif /* FEATURE */
	//scope.log(val, (val - oldVal), val2, (val2 - oldVal2), feature, feature2, 0, 0);
	oldVal = val;
	oldVal2 = val2;
}
#endif /* LOG_KEYS_AUDIO */
#else
#undef LOG_KEYS_AUDIO
#endif /* SCANNER */

bool setup(BelaContext *context, void *userData)
{
#ifdef LOG_KEYS_AUDIO
	scope.setup(4, context->audioSampleRate);
#else /* LOG_KEYS_AUDIO */
	scope.setup(8, 1000);
#endif /* LOG_KEYS_AUDIO */
#ifdef LOWPASS_PIEZOS
	for(unsigned int n = 0; n < gNumPiezos; ++n) {
		piezoFilters.emplace_back(2);
	}
	for(auto &f : piezoFilters)
	{
		for(unsigned int n = 0; n < piezoFiltersCoefficients.size(); ++n)
			f.setCoefficients(piezoFiltersCoefficients[n].data(), n);
	}
#endif /* LOWPASS_PIEZOS */
#ifdef DELAY_INPUTS
	gInputDelaySize = gInputDelayTime * context->audioSampleRate;
	gInputDelayBuffers.resize(gNumPiezos);
	for(auto &v : gInputDelayBuffers)
		v.resize(gInputDelaySize);
#endif /* DELAY_INPUTS */
#ifdef SCANNER
	if(context->digitalFrames != 0)
	{
		fprintf(stderr, "You should disable digitals and run on PRU 0 for the scanner to work.\n");
		return false;
	}

	keys = new Keys;
	bt.setLowestNote(0);
	bt.setBoard(0, 0, 24);
	bt.setBoard(1, 0, 23);
	bt.setBoard(2, 0, 23);
#ifdef FEATURE
	keyFeature.setup(bt.getNumNotes(), 10000);
#endif /* FEATURE */
#ifndef LOG_KEYS_AUDIO
	keys->setPostCallback(postCallback, keys);
#endif /* LOG_KEYS_AUDIO */
	int ret = keys->start(&bt, NULL);
	if(ret < 0)
	{
		fprintf(stderr, "Error while starting the scan of the keys: %d %s\n", ret, strerror(-ret));
		delete keys;
		keys = NULL;
	}
	keys->startTopCalibration();
	keys->loadLinearCalibrationFile("/root/spi-pru/calib.out");
#endif /* SCANNER */
	return true;
}

unsigned int gSampleCount = 0;

void render(BelaContext *context, void *userData)
{
	for(unsigned int n = 0; n < context->audioFrames; ++n)
	{
		++gSampleCount;
#ifdef SCANNER
		if((gSampleCount & 4095) == 0){
			for(int n = bt.getLowestNote(); n <= bt.getHighestNote(); ++n)
				if(n >= gKeyOffset && n < gKeyOffset + gNumPiezos) rt_printf("%.4f ", keys->getNoteValue(n));
			rt_printf("\n");
		}
#endif /* SCANNER */
		int iMax = context->audioInChannels < gNumPiezos ? context->audioInChannels : gNumPiezos;
		for (int i = 0; i < iMax; i++)
		{
#ifdef PIEZOS
			float piezoInput = audioRead(context, n, gMicPin[i]);
		
#ifdef LOWPASS_PIEZOS
			float filteredPiezoInput = piezoFilters[i].process(piezoInput);
			piezoInput = filteredPiezoInput;
#endif /* LOWPASS_PIEZOS */
			// read from delay line
			float input = gInputDelayBuffers[i][gInputDelayIdx];
			// write into delay line
			gInputDelayBuffers[i][gInputDelayIdx] = piezoInput;
#endif /* PIEZOS */
#if (defined SCANNER && defined PIEZOS && LOG_KEYS_AUDIO)
			if(i == 0)
				scope.log(input, keys->getNoteValue(gKeyOffset + i));
#endif /* SCANNER && PIEZOS */
		}
	}
}

void cleanup(BelaContext *context, void *userData)
{
#ifdef SCANNER
	delete keys;
#endif /* SCANNER */

}
