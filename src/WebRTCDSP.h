#ifndef _WEBRTC_EFFECT_H_
#define _WEBRTC_EFFECT_H_

#include "AudioEffect.h"

namespace webrtc {
class AudioProcessing;
}

namespace SpeexWebRTCTest {

class WebRTCDSP final : public AudioEffect
{
	Q_OBJECT
public:
	WebRTCDSP(const QAudioFormat& mainFormat, const QAudioFormat& auxFormat);
	~WebRTCDSP() override;

	void processFrame(QAudioBuffer& mainBuffer, const QAudioBuffer& auxBuffer) override;
	void setParameter(const QString& param, QVariant value) override;

private:
	unsigned int requiredFrameSizeMs() const override;

	webrtc::AudioProcessing* apm_;
};

} // namespace SpeexWebRTCTest

#endif // _WEBRTC_EFFECT_H_
