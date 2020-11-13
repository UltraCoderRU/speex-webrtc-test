#include "AudioProcessor.h"

#include "SpeexDSP.h"
#include "Timer.h"
#include "WebRTCDSP.h"

#include <QAudioBuffer>
#include <QLoggingCategory>

namespace SpeexWebRTCTest {

namespace {
Q_LOGGING_CATEGORY(processor, "processor")
}

QVector<qreal> calculateAudioLevels(const QAudioBuffer& buffer);

QByteArray popFront(QByteArray& buffer, const std::size_t size)
{
	QByteArray result;
	result.append(buffer.data(), size);
	buffer.remove(0, size);
	return result;
}

AudioProcessor::AudioProcessor(const QAudioFormat& format,
                               const QAudioFormat& monitorFormat,
                               QBuffer& monitorDevice,
                               QObject* parent)
    : QIODevice(parent),
      bufferSize_(1024),
      format_(format),
      monitorFormat_(monitorFormat),
      monitorDevice_(monitorDevice)
{
	qRegisterMetaType<QVector<qreal>>();

	switchBackend(Backend::Speex);

	connect(&monitorDevice_, &QIODevice::readyRead,
	        [this]
	        {
		        monitorDevice_.seek(0);
		        {
			        std::unique_lock<std::mutex> lock(monitorMutex_);
			        monitorBuffer_.append(monitorDevice_.readAll());
		        }
		        monitorDevice_.buffer().clear();
		        monitorDevice_.seek(0);
	        });

	doWork_ = true;
	worker_ = std::thread([this] { process(); });
}

AudioProcessor::~AudioProcessor()
{
	doWork_ = false;
	worker_.join();
}

qint64 AudioProcessor::readData(char* data, qint64 maxlen)
{
	std::unique_lock<std::mutex> lock(outputMutex_);
	int len = std::min((qint64)outputBuffer_.size(), maxlen);
	QByteArray buffer = popFront(outputBuffer_, len);
	memcpy(data, buffer.data(), len);
	return len;
}

qint64 AudioProcessor::writeData(const char* data, qint64 len)
{
	std::unique_lock<std::mutex> lock1(inputMutex_);
	std::unique_lock<std::mutex> lock2(inputEventMutex_);
	inputBuffer_.append(data, len);
	inputEvent_.notify_all();
	return len;
}

void AudioProcessor::process()
{
	while (doWork_)
	{
		std::unique_lock<std::mutex> processLock(processMutex_);

		const std::size_t bytesToRead =
		    bufferSize_ * format_.sampleSize() / 8 * format_.channelCount();
		const std::size_t monitorToRead =
		    bufferSize_ * monitorFormat_.sampleSize() / 8 * monitorFormat_.channelCount();

		QByteArray buffer;
		QByteArray monitorBuffer;

		{
			std::unique_lock<std::mutex> lock(inputMutex_);
			if (inputBuffer_.size() >= bytesToRead)
				buffer = popFront(inputBuffer_, bytesToRead);
		}

		if (!buffer.isEmpty())
		{
			{
				std::unique_lock<std::mutex> lock(monitorMutex_);
				if (monitorBuffer_.size() >= (monitorToRead))
					monitorBuffer = popFront(monitorBuffer_, monitorToRead);
				else
					monitorBuffer = QByteArray(monitorToRead, 0);
			}

			QAudioBuffer buf(buffer, format_);
			QAudioBuffer monitorBuf(monitorBuffer, monitorFormat_);

			if (sourceEncoder_->isOpen())
				sourceEncoder_->write(buf.constData<char>(), buf.byteCount());

			processBuffer(buf, monitorBuf);

			if (processedEncoder_->isOpen())
				processedEncoder_->write(buf.constData<char>(), buf.byteCount());

			{
				std::unique_lock<std::mutex> lock(outputMutex_);
				outputBuffer_.append(buf.constData<char>(), buf.byteCount());
				emit readyRead();
			}
		}
		else
		{
			processLock.unlock();
			std::unique_lock<std::mutex> lock(inputEventMutex_);
			inputEvent_.wait(lock);
		}
	}
}

void AudioProcessor::processBuffer(QAudioBuffer& inputBuffer, const QAudioBuffer& monitorBuffer)
{
	TIMER(qDebug(processor))

	QVector<qreal> inputLevels = calculateAudioLevels(inputBuffer);

	if (dsp_)
		dsp_->processFrame(inputBuffer, monitorBuffer);

	QVector<qreal> outputLevels = calculateAudioLevels(inputBuffer);

	emit inputLevelsChanged(inputLevels);
	emit outputLevelsChanged(outputLevels);
}

void AudioProcessor::clearBuffers()
{
	{
		std::unique_lock<std::mutex> lock(inputMutex_);
		inputBuffer_.clear();
	}
	{
		std::unique_lock<std::mutex> lock(outputMutex_);
		outputBuffer_.clear();
	}
	{
		std::unique_lock<std::mutex> lock(monitorMutex_);
		monitorBuffer_.clear();
	}
}

bool AudioProcessor::isSequential() const
{
	return true;
}

bool AudioProcessor::canReadLine() const
{
	return false;
}

qint64 AudioProcessor::bytesAvailable() const
{
	std::unique_lock<std::mutex> lock(outputMutex_);
	return outputBuffer_.size();
}

bool AudioProcessor::open(QIODevice::OpenMode mode)
{
	std::unique_lock<std::mutex> lock(processMutex_);
	clearBuffers();

	sourceEncoder_.reset(new WavFileWriter("source.wav", format_));
	sourceEncoder_->open();

	processedEncoder_.reset(new WavFileWriter("processed.wav", format_));
	processedEncoder_->open();

	return QIODevice::open(mode);
}

void AudioProcessor::close()
{
	if (sourceEncoder_)
	{
		sourceEncoder_->close();
		sourceEncoder_.reset();
	}
	if (processedEncoder_)
	{
		processedEncoder_->close();
		processedEncoder_.reset();
	}
	QIODevice::close();
}

Backend AudioProcessor::getCurrentBackend() const
{
	if (dynamic_cast<SpeexDSP*>(dsp_.get()))
		return Backend::Speex;
	else
		return Backend::WebRTC;
}

void AudioProcessor::switchBackend(Backend backend)
{
	std::unique_lock<std::mutex> lock(processMutex_);
	clearBuffers();

	if (backend == Backend::Speex)
		dsp_.reset(new SpeexDSP(format_, monitorFormat_));
	else
		dsp_.reset(new WebRTCDSP(format_, monitorFormat_));

	bufferSize_ = dsp_->getFrameSize();

	connect(dsp_.get(), &AudioEffect::voiceActivityChanged, this,
	        &AudioProcessor::voiceActivityChanged);
}

void AudioProcessor::setEffectParam(const QString& param, const QVariant& value)
{
	dsp_->setParameter(param, value);
}

////////////////////////////////////////////////////////////

// This function returns the maximum possible sample value for a given audio format
qreal getPeakValue(const QAudioFormat& format)
{
	// Note: Only the most common sample formats are supported
	if (!format.isValid())
		return qreal(0);

	if (format.codec() != "audio/pcm")
		return qreal(0);

	switch (format.sampleType())
	{
	case QAudioFormat::Unknown:
		break;
	case QAudioFormat::Float:
		if (format.sampleSize() != 32) // other sample formats are not supported
			return qreal(0);
		return qreal(1.00003);
	case QAudioFormat::SignedInt:
		if (format.sampleSize() == 32)
			return qreal(INT_MAX);
		if (format.sampleSize() == 16)
			return qreal(SHRT_MAX);
		if (format.sampleSize() == 8)
			return qreal(CHAR_MAX);
		break;
	case QAudioFormat::UnSignedInt:
		if (format.sampleSize() == 32)
			return qreal(UINT_MAX);
		if (format.sampleSize() == 16)
			return qreal(USHRT_MAX);
		if (format.sampleSize() == 8)
			return qreal(UCHAR_MAX);
		break;
	}

	return qreal(0);
}

template <class T>
QVector<qreal> getBufferLevels(const T* buffer, int frames, int channels)
{
	QVector<qreal> max_values;
	max_values.fill(0, channels);

	for (int i = 0; i < frames; ++i)
	{
		for (int j = 0; j < channels; ++j)
		{
			qreal value = qAbs(qreal(buffer[i * channels + j]));
			if (value > max_values.at(j))
				max_values.replace(j, value);
		}
	}

	return max_values;
}

QVector<qreal> calculateAudioLevels(const QAudioBuffer& buffer)
{
	QVector<qreal> values;

	if (!buffer.format().isValid() || buffer.format().byteOrder() != QAudioFormat::LittleEndian)
		return values;

	if (buffer.format().codec() != "audio/pcm")
		return values;

	int channelCount = buffer.format().channelCount();
	values.fill(0, channelCount);
	qreal peak_value = getPeakValue(buffer.format());
	if (qFuzzyCompare(peak_value, qreal(0)))
		return values;

	switch (buffer.format().sampleType())
	{
	case QAudioFormat::Unknown:
	case QAudioFormat::UnSignedInt:
		if (buffer.format().sampleSize() == 32)
			values = getBufferLevels(buffer.constData<quint32>(), buffer.frameCount(), channelCount);
		if (buffer.format().sampleSize() == 16)
			values = getBufferLevels(buffer.constData<quint16>(), buffer.frameCount(), channelCount);
		if (buffer.format().sampleSize() == 8)
			values = getBufferLevels(buffer.constData<quint8>(), buffer.frameCount(), channelCount);
		for (int i = 0; i < values.size(); ++i)
			values[i] = qAbs(values.at(i) - peak_value / 2) / (peak_value / 2);
		break;
	case QAudioFormat::Float:
		if (buffer.format().sampleSize() == 32)
		{
			values = getBufferLevels(buffer.constData<float>(), buffer.frameCount(), channelCount);
			for (int i = 0; i < values.size(); ++i)
				values[i] /= peak_value;
		}
		break;
	case QAudioFormat::SignedInt:
		if (buffer.format().sampleSize() == 32)
			values = getBufferLevels(buffer.constData<qint32>(), buffer.frameCount(), channelCount);
		if (buffer.format().sampleSize() == 16)
			values = getBufferLevels(buffer.constData<qint16>(), buffer.frameCount(), channelCount);
		if (buffer.format().sampleSize() == 8)
			values = getBufferLevels(buffer.constData<qint8>(), buffer.frameCount(), channelCount);
		for (int i = 0; i < values.size(); ++i)
			values[i] /= peak_value;
		break;
	}

	// Convert to dBFS
	for (auto& level : values)
		level = QAudio::convertVolume(level, QAudio::LinearVolumeScale, QAudio::DecibelVolumeScale);

	return values;
}

} // namespace SpeexWebRTCTest
