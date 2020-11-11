#include "WebRTCDSP.h"

#include <webrtc/modules/audio_processing/include/audio_processing.h>
#include <webrtc/modules/interface/module_common_types.h>

#include <QLoggingCategory>

namespace SpeexWebRTCTest {

namespace {

Q_LOGGING_CATEGORY(WebRTC, "webrtc")

void convert(const QAudioBuffer& from, webrtc::AudioFrame& to)
{
	to.num_channels_ = from.format().channelCount();
	to.sample_rate_hz_ = from.format().sampleRate();
	to.samples_per_channel_ = from.frameCount();
	to.interleaved_ = (from.format().channelCount() > 1);
	memcpy(to.data_, from.constData<char>(), from.byteCount());
}

void convert(const webrtc::AudioFrame& from, QAudioBuffer& to)
{
	QAudioFormat format;
	format.setSampleRate(from.sample_rate_hz_);
	format.setChannelCount(from.num_channels_);
	format.setSampleSize(16);
	format.setCodec("audio/pcm");
	format.setByteOrder(QAudioFormat::LittleEndian);
	format.setSampleType(QAudioFormat::SignedInt);

	QByteArray data(reinterpret_cast<const char*>(from.data_),
	                from.samples_per_channel_ * from.num_channels_ * sizeof(std::int16_t));
	to = QAudioBuffer(data, format);
}

} // namespace

WebRTCDSP::WebRTCDSP(const QAudioFormat& mainFormat, const QAudioFormat& auxFormat)
    : AudioEffect(mainFormat, auxFormat)
{
	apm_ = webrtc::AudioProcessing::Create();
	if (!apm_)
		throw std::runtime_error("failed to create webrtc::AudioProcessing instance");

	webrtc::Config config;
	config.Set<webrtc::DelayAgnostic>(new webrtc::DelayAgnostic(true));
	config.Set<webrtc::ExtendedFilter>(new webrtc::ExtendedFilter(true));
	apm_->SetExtraOptions(config);

	apm_->voice_detection()->Enable(true);
	apm_->voice_detection()->set_frame_size_ms(300);
	apm_->voice_detection()->set_likelihood(webrtc::VoiceDetection::kModerateLikelihood);

	apm_->noise_suppression()->Enable(false);
	apm_->noise_suppression()->set_level(webrtc::NoiseSuppression::kLow);

	apm_->echo_cancellation()->Enable(false);
	apm_->echo_cancellation()->set_suppression_level(webrtc::EchoCancellation::kLowSuppression);
	apm_->set_stream_delay_ms(100);

	apm_->gain_control()->Enable(false);
	apm_->gain_control()->set_mode(webrtc::GainControl::kAdaptiveDigital);
	apm_->gain_control()->enable_limiter(true);
	apm_->gain_control()->set_compression_gain_db(0);
	apm_->gain_control()->set_target_level_dbfs(0);

	// apm_->Initialize();
}

WebRTCDSP::~WebRTCDSP()
{
	delete apm_;
}

QString errorDescription(int error)
{
	switch (error)
	{
	case webrtc::AudioProcessing::kNoError:
		return "no error";
	case webrtc::AudioProcessing::kUnspecifiedError:
		return "unspecified error";
	case webrtc::AudioProcessing::kCreationFailedError:
		return "creation failed";
	case webrtc::AudioProcessing::kUnsupportedComponentError:
		return "unsupported component";
	case webrtc::AudioProcessing::kUnsupportedFunctionError:
		return "unsupported function";
	case webrtc::AudioProcessing::kNullPointerError:
		return "null pointer";
	case webrtc::AudioProcessing::kBadParameterError:
		return "bad parameter";
	case webrtc::AudioProcessing::kBadSampleRateError:
		return "bad sample rate";
	case webrtc::AudioProcessing::kBadDataLengthError:
		return "bad data length";
	case webrtc::AudioProcessing::kBadNumberChannelsError:
		return "bad number of channels";
	case webrtc::AudioProcessing::kFileError:
		return "file error";
	case webrtc::AudioProcessing::kStreamParameterNotSetError:
		return "stream parameter not set";
	case webrtc::AudioProcessing::kNotEnabledError:
		return "not enabled";
	case webrtc::AudioProcessing::kBadStreamParameterWarning:
		return "bad stream parameter (non-fatal)";
	default:
		return "unexpected error";
	}
}

void WebRTCDSP::processFrame(QAudioBuffer& mainBuffer, const QAudioBuffer& auxBuffer)
{
	qDebug(WebRTC).noquote() << QString(
	                                "got %1 near-end samples at %2 Hz (%3ms) and %4 far-end "
	                                "samples at %5 (%6ms)")
	                                .arg(mainBuffer.frameCount())
	                                .arg(mainBuffer.format().sampleRate())
	                                .arg(mainBuffer.duration() / 1000)
	                                .arg(auxBuffer.frameCount())
	                                .arg(auxBuffer.format().sampleRate())
	                                .arg(auxBuffer.duration() / 1000);

	webrtc::AudioFrame mainFrame, auxFrame;
	convert(mainBuffer, mainFrame);
	convert(auxBuffer, auxFrame);

	Q_ASSERT(mainFrame.sample_rate_hz_ == auxFrame.sample_rate_hz_);
	Q_ASSERT(mainFrame.sample_rate_hz_ == mainBuffer.format().sampleRate());

	int error;

	if (apm_->echo_cancellation()->is_enabled())
	{
		error = apm_->ProcessReverseStream(&auxFrame);
		if (error != 0)
		{
			qWarning(WebRTC).noquote() << "ProcessReverseStream() error:" << errorDescription(error);
		}

		apm_->set_stream_delay_ms(100);
		// apm_->echo_cancellation()->set_stream_drift_samples(50);
	}

	error = apm_->ProcessStream(&mainFrame);
	if (error != 0)
	{
		qCritical(WebRTC).noquote() << "ProcessStream() error:" << errorDescription(error);
		return;
	}

	convert(mainFrame, mainBuffer);

	setVoiceActive(apm_->voice_detection()->stream_has_voice());
}

void WebRTCDSP::setParameter(const QString& param, QVariant value)
{
	if (param == "noise_reduction_enabled")
		apm_->noise_suppression()->Enable(value.toBool());
	else if (param == "noise_reduction_suppression_level")
		apm_->noise_suppression()->set_level(static_cast<webrtc::NoiseSuppression::Level>(
		    webrtc::NoiseSuppression::kLow + value.toUInt()));
	else if (param == "echo_cancellation_enabled")
		apm_->echo_cancellation()->Enable(value.toBool());
	else if (param == "echo_cancellation_suppression_level")
		apm_->echo_cancellation()->set_suppression_level(
		    static_cast<webrtc::EchoCancellation::SuppressionLevel>(
		        webrtc::EchoCancellation::kLowSuppression + value.toUInt()));
	else if (param == "gain_control_enabled")
		apm_->gain_control()->Enable(value.toBool());
	else if (param == "gain_control_target_level")
		apm_->gain_control()->set_target_level_dbfs(value.toInt());
	else if (param == "gain_control_max_gain")
		apm_->gain_control()->set_compression_gain_db(value.toInt());
	else
		throw std::invalid_argument("Invalid param");
}

unsigned int WebRTCDSP::requiredFrameSizeMs() const
{
	return 10;
}

} // namespace SpeexWebRTCTest
