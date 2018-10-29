#include <Keys.h>

class Percussiveness
{
public:
	void setBufferLength(unsigned int length);
	float process(float input);
	float getFeature() { return feature; };
private:
	float oldPosition = 0;
	float oldVelocity = 0;
	float oldOldVelocity = 0;
	std::vector<float> inputBuffer;
	float feature;
	float accThreshold = 0.01;
};

class KeyFeature
{
public:
	void setup(unsigned int newNumKeys, unsigned int bufferLength);
	static void postCallback(void* arg, float* buffer, unsigned int length);
	std::vector<Percussiveness> percussiveness;
private:
	unsigned int numKeys;
};

