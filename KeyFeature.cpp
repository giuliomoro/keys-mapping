#include "KeyFeature.h"
#include <algorithm>

void Percussiveness::setBufferLength(unsigned int length)
{
	inputBuffer.resize(length, 0);
}

float Percussiveness::process(float input)
{
	// write to delay line

	// if smaller than activation threshold, look back lookBack samples
	// to check when the press started

	// look for a rebound: the velocity changes direction above a threshold

	float position = input;
	float velocity = position - oldPosition;
	if(oldOldVelocity <= oldVelocity && oldVelocity >= oldVelocity)
	{
		// peak
		//feature = oldVelocity - oldOldVelocity;
		if(oldVelocity <= 0 && velocity > 0 
				//&& velocity - oldVelocity > accThreshold
				)
		{
			feature = oldVelocity;
		}
	}

	oldPosition = position;
	oldOldVelocity = oldVelocity;
	oldVelocity = velocity;
	return feature;
}

void KeyFeature::postCallback(void* arg, float* buffer, unsigned int length)
{
	KeyFeature* that = (KeyFeature*)arg;
	for(unsigned int n = 0; n < std::min(that->numKeys, length); ++n)
	{
		that->percussiveness[n].process(buffer[n]);
	}
}

void KeyFeature::setup(unsigned int newNumKeys, unsigned int bufferLength)
{
	numKeys = newNumKeys;
	percussiveness.resize(numKeys);
	for(auto& p : percussiveness)
		p.setBufferLength(bufferLength);
}

