#include "AudioEffect.h"

namespace SpeexWebRTCTest {

AudioEffect::AudioEffect(unsigned int frameSize,
                         const QAudioFormat& mainFormat,
                         const QAudioFormat& auxFormat)
    : frameSize_(frameSize), mainFormat_(mainFormat), auxFormat_(auxFormat)
{
}

unsigned int AudioEffect::getFrameSize() const
{
	return frameSize_;
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
