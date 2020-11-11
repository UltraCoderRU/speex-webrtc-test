#ifndef _SPEEXDSP_EFFECT_H_
#define _SPEEXDSP_EFFECT_H_

#include "AudioEffect.h"

struct SpeexPreprocessState_;
typedef struct SpeexPreprocessState_ SpeexPreprocessState;
struct SpeexEchoState_;
typedef struct SpeexEchoState_ SpeexEchoState;

namespace SpeexWebRTCTest {

class SpeexDSP final : public AudioEffect
{
	Q_OBJECT
public:
	SpeexDSP(const QAudioFormat& mainFormat, const QAudioFormat& auxFormat);
	~SpeexDSP() override;

	void processFrame(QAudioBuffer& mainBuffer, const QAudioBuffer& auxBuffer) override;
	void setParameter(const QString& param, QVariant value) override;

private:
	unsigned int requiredFrameSizeMs() const override;

	SpeexPreprocessState* preprocess_ = nullptr;
	SpeexEchoState* echo_ = nullptr;

	bool aecEnabled = false;
};

} // namespace SpeexWebRTCTest

#endif // _SPEEXDSP_EFFECT_H_
