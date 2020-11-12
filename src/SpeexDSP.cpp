#include "SpeexDSP.h"

#include "Timer.h"

#include <speex/speex_echo.h>
#include <speex/speex_preprocess.h>

#include <QLoggingCategory>

namespace SpeexWebRTCTest {

namespace {
Q_LOGGING_CATEGORY(Speex, "speex")
int on = 1;
int off = 0;
} // namespace

SpeexDSP::SpeexDSP(const QAudioFormat& mainFormat, const QAudioFormat& auxFormat)
    : AudioEffect(mainFormat, auxFormat)
{
	preprocess_ = speex_preprocess_state_init(getFrameSize(), getMainFormat().sampleRate());
	echo_ = speex_echo_state_init_mc(getFrameSize(), getFrameSize() * 10,
	                                 getMainFormat().channelCount(), getAuxFormat().channelCount());

	speex_preprocess_ctl(preprocess_, SPEEX_PREPROCESS_SET_VAD, &on);
	speex_preprocess_ctl(preprocess_, SPEEX_PREPROCESS_SET_DENOISE, &off);
	speex_preprocess_ctl(preprocess_, SPEEX_PREPROCESS_SET_AGC, &off);

	std::int32_t sampleRate = getMainFormat().sampleRate();
	speex_echo_ctl(echo_, SPEEX_ECHO_SET_SAMPLING_RATE, &sampleRate);
}

SpeexDSP::~SpeexDSP()
{
	speex_preprocess_state_destroy(preprocess_);
	speex_echo_state_destroy(echo_);
}

void SpeexDSP::processFrame(QAudioBuffer& mainBuffer, const QAudioBuffer& auxBuffer)
{
	TIMER(qDebug(Speex))

	qDebug(Speex).noquote() << QString(
	                               "got %1 near-end samples (%2ms) and %3 far-end samples (%4ms)")
	                               .arg(mainBuffer.frameCount())
	                               .arg(mainBuffer.duration() / 1000)
	                               .arg(auxBuffer.frameCount())
	                               .arg(auxBuffer.duration() / 1000);


	Q_ASSERT(mainBuffer.frameCount() == auxBuffer.frameCount());

	bool voiceActive = (speex_preprocess_run(preprocess_, mainBuffer.data<spx_int16_t>()) == 1);
	setVoiceActive(voiceActive);

	if (aecEnabled)
		speex_echo_cancellation(echo_, mainBuffer.data<spx_int16_t>(),
		                        auxBuffer.data<spx_int16_t>(), mainBuffer.data<spx_int16_t>());
}

void SpeexDSP::setParameter(const QString& param, QVariant value)
{
	if (param == "noise_reduction_enabled")
		speex_preprocess_ctl(preprocess_, SPEEX_PREPROCESS_SET_DENOISE, value.data());
	else if (param == "noise_reduction_max_attenuation")
		speex_preprocess_ctl(preprocess_, SPEEX_PREPROCESS_SET_NOISE_SUPPRESS, value.data());
	else if (param == "echo_cancellation_enabled")
		aecEnabled = value.toBool();
	else if (param == "echo_cancellation_max_attenuation")
		speex_preprocess_ctl(preprocess_, SPEEX_PREPROCESS_SET_ECHO_SUPPRESS, value.data());
	else if (param == "gain_control_enabled")
		speex_preprocess_ctl(preprocess_, SPEEX_PREPROCESS_SET_AGC, value.data());
	else if (param == "gain_control_level")
		speex_preprocess_ctl(preprocess_, SPEEX_PREPROCESS_SET_AGC_TARGET, value.data());
	else if (param == "gain_control_max_gain")
		speex_preprocess_ctl(preprocess_, SPEEX_PREPROCESS_SET_AGC_MAX_GAIN, value.data());
	else if (param == "gain_control_max_increment")
		speex_preprocess_ctl(preprocess_, SPEEX_PREPROCESS_SET_AGC_INCREMENT, value.data());
	else if (param == "gain_control_max_decrement")
		speex_preprocess_ctl(preprocess_, SPEEX_PREPROCESS_SET_AGC_DECREMENT, value.data());
	else
		throw std::invalid_argument("Invalid param");
}

unsigned int SpeexDSP::requiredFrameSizeMs() const
{
	return 25;
}

} // namespace SpeexWebRTCTest
