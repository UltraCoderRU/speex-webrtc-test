#ifndef _AUDIO_PROCESSOR_H_
#define _AUDIO_PROCESSOR_H_

#include "AudioEffect.h"
#include "WavFileWriter.h"

#include <QAudioFormat>
#include <QBuffer>
#include <QIODevice>
#include <QScopedPointer>

#include <mutex>
#include <thread>

class QAudioBuffer;

namespace SpeexWebRTCTest {

enum class Backend
{
	Speex,
	WebRTC
};

class AudioProcessor final : public QIODevice
{
	Q_OBJECT
public:
	explicit AudioProcessor(const QAudioFormat& format,
	                        const QAudioFormat& monitorFormat,
	                        QBuffer& monitorDevice,
	                        QObject* parent = nullptr);

	~AudioProcessor() override;

	bool open(OpenMode mode) override;
	void close() override;

	qint64 bytesAvailable() const override;
	bool isSequential() const override;
	bool canReadLine() const override;

	Backend getCurrentBackend() const;
	void switchBackend(Backend);

	void setEffectParam(const QString& param, const QVariant& value);

signals:
	void voiceActivityChanged(bool);
	void inputLevelsChanged(const QVector<qreal>&);
	void outputLevelsChanged(const QVector<qreal>&);

protected:
	qint64 readData(char* data, qint64 maxlen) override;
	qint64 writeData(const char* data, qint64 len) override;

private:
	void process();
	void processBuffer(QAudioBuffer& inputBuffer, const QAudioBuffer& monitorBuffer);

	mutable std::mutex inputMutex_;
	mutable std::mutex outputMutex_;
	mutable std::mutex monitorMutex_;

	std::size_t bufferSize_;
	const QAudioFormat format_;
	const QAudioFormat monitorFormat_;
	QBuffer& monitorDevice_;

	QByteArray inputBuffer_;
	QByteArray monitorBuffer_;
	QByteArray outputBuffer_;

	QScopedPointer<AudioEffect> dsp_;

	std::thread worker_;
	bool doWork_ = false;

	QScopedPointer<WavFileWriter> sourceEncoder_;
	QScopedPointer<WavFileWriter> processedEncoder_;
};

} // namespace SpeexWebRTCTest

Q_DECLARE_METATYPE(QVector<qreal>)

#endif // _AUDIO_PROCESSOR_H_
