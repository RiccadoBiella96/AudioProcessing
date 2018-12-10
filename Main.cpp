#include "../JuceLibraryCode/JuceHeader.h"
#include "MyFilter.h"
#include "CircularBuffer.h"
#include "Tone.h"
#include <cassert>

int main(int argc, char **argv)
{
	File input;

	if (argc != 2) {
		input = File::getCurrentWorkingDirectory().getChildFile("SignalToAnalyse.wav");
		//std::cerr << "Missing input audio file to analyze." << std::endl;
		//return 1;
	}
	else {
		input = File::getCurrentWorkingDirectory().getChildFile(argv[1]);
	}

	
	if (input.exists() == false) {
		std::cerr << "File doesn't exists." << std::endl;
		return 1;
	}

	AudioFormatManager fmgr;
	fmgr.registerBasicFormats();
	ScopedPointer<AudioFormatReaderSource> source = new AudioFormatReaderSource(fmgr.createReaderFor(input), true);
	
	unsigned int numChannels = source->getAudioFormatReader()->numChannels;
	double sampleRate = source->getAudioFormatReader()->sampleRate;

	/* Declaration of the necessary variables */
	const int FFTsize{ 1024 };
	const int numTones = 12;
	int observations[numTones] = { 0 };
	int buttonsDetected[numTones] = { 0 };
	int silence = 0;
	const int silenceThreshold = 120;
	const int buttonThreshold = 12;

	/* Declaration of a buffer in which to copy samples read from files*/
	float buffer[FFTsize * 2];

	/* Declaration of a circular buffer and a filter */
	CircularBuffer<float> circularBuffer{ FFTsize, 64 };
	MyFilter<float> filter{ 690, 1650, sampleRate};

	/* Create a vector in which to store the DTMF frequences values */
	std::vector<Tone> tones;

	tones.push_back(Tone(697.f, 1209.f, '1', ""));
	tones.push_back(Tone(697.f, 1336.f, '2', "ABC"));
	tones.push_back(Tone(697.f, 1477.f, '3', "DEF"));
	tones.push_back(Tone(770.f, 1209.f, '4', "GHI"));
	tones.push_back(Tone(770.f, 1336.f, '5', "JKL"));
	tones.push_back(Tone(770.f, 1477.f, '6', "MNO"));
	tones.push_back(Tone(852.f, 1209.f, '7', "PQRS"));
	tones.push_back(Tone(852.f, 1336.f, '8', "TUV"));
	tones.push_back(Tone(852.f, 1477.f, '9', "WXYZ"));
	tones.push_back(Tone(941.f, 1209.f, '*', ""));
	tones.push_back(Tone(941.f, 1336.f, '0', " "));
	tones.push_back(Tone(941.f, 1477.f, '#', ""));

	std::cout << "Channels=" << numChannels << " sampleRate=" << (int)sampleRate << std::endl;

	AudioBuffer<float> myBuffer(numChannels, sampleRate / 10);
	AudioSourceChannelInfo info(myBuffer);

	source->prepareToPlay(512, sampleRate);

	while (source->getNextReadPosition() < source->getTotalLength())
	{
		// Read next audio block
		source->getNextAudioBlock(info);

		const int numSamples = info.buffer->getNumSamples();
		const int numChannels = info.buffer->getNumChannels();
		const float **data = info.buffer->getArrayOfReadPointers();

		// loop through each channel
		for (int channel = 0; channel < numChannels; channel++)
		{
			// channelData contains now 'numSamples' of input data
			const float *channelData = data[channel];

			//TODO implement you algorithm! The following example dumps to stdout the audio contents

			for (int sampleIndex = 0; sampleIndex < numSamples; sampleIndex++) {
				
				if (circularBuffer.process(channelData[sampleIndex])) {

					/* Copy the content of circular buffer in buffer in order to execute the FFT*/
					circularBuffer.copyData(buffer);

					/* Signal cleaning */
					filter.process(buffer, FFTsize);

					/* FFT execution */
					dsp::FFT fft{ 10 };
					fft.performFrequencyOnlyForwardTransform(buffer);

					/* Convertion of the spectre result in dB */
					// fftSize → dimensione FFT
					// buffer → array di dimensione 2 * fftSize contenente il risultato della FFT
					buffer[0] = 20.0f * log10f(buffer[0] / (float)FFTsize / 2.0f);
					for (int i = 1; i < FFTsize / 2; i++) {
						buffer[i] = 20.0f * log10f((2.0f * buffer[i]) / (float)FFTsize / 2.0f);
					}

					bool found = false;
					for (int i = 0; i < numTones; i++) {

						Tone t = tones.at(i);
						if (t.lookup(buffer, FFTsize / 2)) {

							found = true;

							/* Silence detected */
							if (silence > silenceThreshold) {
								for (int j = 0; j < numTones; j++) {
									if (buttonsDetected[j] != 0) std::cout << tones.at(j).getChar(buttonsDetected[j]);
									observations[j] = 0;
									buttonsDetected[j] = 0;
								}
							}
							silence = 0;

							/* observation stored */
							observations[i]++;

							/* Pressing button event registered */
							if (observations[i] > buttonThreshold) {
								buttonsDetected[i]++;
								for (int j = 0; j < numTones; j++)
									observations[j] = 0;
							}
						}
					}

					/* Tone not detected */
					if (!found) { 
						silence++;
					}
				}
			}
		}
	}

	/* Management of the last character */
	for (int j = 0; j < numTones; j++) 
		if (buttonsDetected[j] != 0) std::cout << tones.at(j).getChar(buttonsDetected[j]);

	return 0;
}


