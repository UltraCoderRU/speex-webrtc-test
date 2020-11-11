#include "AudioEffect.h"

namespace SpeexWebRTCTest {

AudioEffect::AudioEffect(const QAudioFormat& mainFormat, const QAudioFormat& auxFormat)
    : mainFormat_(mainFormat), auxFormat_(auxFormat)
{
}

unsigned int AudioEffect::getFrameSize() const
{
	return mainFormat_.sampleRate() * requiredFrameSizeMs() / 1000;
}

const QAudioFormat& AudioEffect::getMainFormat() const
{
	return mainFormat_;
}

const QAudioFormat& AudioEffect::getAuxFormat() const
{
	return auxFormat_;
}

} // namespace SpeexWebRTCTest
