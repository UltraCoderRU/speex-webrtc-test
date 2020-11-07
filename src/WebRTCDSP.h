#ifndef _WEBRTC_EFFECT_H_
#define _WEBRTC_EFFECT_H_

#include "AudioEffect.h"

namespace webrtc {
class AudioProcessing;
}

namespace SpeexWebRTCTest {

class WebRTCDSP : public AudioEffect
{
	Q_OBJECT
public:
	WebRTCDSP(unsigned int frameSize, const QAudioFormat& mainFormat, const QAudioFormat& auxFormat);
	~WebRTCDSP() override;

	void processFrame(QAudioBuffer& mainBuffer, const QAudioBuffer& auxBuffer) override;
	void setParameter(const QString& param, QVariant value) override;

private:
	webrtc::AudioProcessing* apm_;
};

} // namespace SpeexWebRTCTest

#endif // _WEBRTC_EFFECT_H_
