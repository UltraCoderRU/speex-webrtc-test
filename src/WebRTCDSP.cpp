#include "WebRTCDSP.h"

#include <webrtc/api/audio/audio_frame.h>
#include <webrtc/modules/audio_processing/include/audio_processing.h>

#include <QLoggingCategory>

namespace SpeexWebRTCTest {

namespace {

using NoiseSuppressionLevel = webrtc::AudioProcessing::Config::NoiseSuppression::Level;

Q_LOGGING_CATEGORY(WebRTC, "webrtc")

void convert(const QAudioBuffer& from, webrtc::AudioFrame& to)
{
	to.num_channels_ = from.format().channelCount();
	to.sample_rate_hz_ = from.format().sampleRate();
	to.samples_per_channel_ = from.frameCount();
	memcpy(to.mutable_data(), from.constData<char>(), from.byteCount());
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

	QByteArray data(reinterpret_cast<const char*>(from.data()),
	                from.samples_per_channel_ * from.num_channels_ * sizeof(std::int16_t));
	to = QAudioBuffer(data, format);
}

} // namespace

WebRTCDSP::WebRTCDSP(const QAudioFormat& mainFormat, const QAudioFormat& auxFormat)
    : AudioEffect(mainFormat, auxFormat)
{
	apm_ = webrtc::AudioProcessingBuilder().Create();

	if (!apm_)
		throw std::runtime_error("failed to create webrtc::AudioProcessing instance");

	webrtc::AudioProcessing::Config config;

	config.voice_detection.enabled = true;

	config.noise_suppression.enabled = false;
	config.noise_suppression.level = NoiseSuppressionLevel::kLow;

	config.echo_canceller.enabled = false;
	config.echo_canceller.mobile_mode = false;
	config.residual_echo_detector.enabled = false;

	config.gain_controller1.enabled = false;
	config.gain_controller1.mode = webrtc::AudioProcessing::Config::GainController1::kAdaptiveDigital;
	config.gain_controller1.enable_limiter = true;
	config.gain_controller1.compression_gain_db = 0;
	config.gain_controller1.target_level_dbfs = 0;

	apm_->ApplyConfig(config);
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

	if (apm_->GetConfig().echo_canceller.enabled)
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

	setVoiceActive(*apm_->GetStatistics().voice_detected);
}

void WebRTCDSP::setParameter(const QString& param, QVariant value)
{
	auto config = apm_->GetConfig();
	if (param == "noise_reduction_enabled")
		config.noise_suppression.enabled = value.toBool();
	else if (param == "noise_reduction_suppression_level")
		config.noise_suppression.level =
		    static_cast<NoiseSuppressionLevel>(NoiseSuppressionLevel::kLow + value.toUInt());
	else if (param == "echo_cancellation_enabled")
		config.echo_canceller.enabled = value.toBool();
	else if (param == "echo_cancellation_suppression_level")
		return; // TODO ???
	else if (param == "gain_control_enabled")
		config.gain_controller1.enabled = value.toBool();
	else if (param == "gain_control_target_level")
		config.gain_controller1.target_level_dbfs = value.toInt();
	else if (param == "gain_control_max_gain")
		config.gain_controller1.compression_gain_db = value.toInt();
	else
		throw std::invalid_argument("Invalid param");
	apm_->ApplyConfig(config);
}

unsigned int WebRTCDSP::requiredFrameSizeMs() const
{
	return 10;
}

} // namespace SpeexWebRTCTest
