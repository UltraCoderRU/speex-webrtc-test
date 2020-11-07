#ifndef _WAV_FILE_WRITER_H_
#define _WAV_FILE_WRITER_H_

#include <QtCore>
#include <QtMultimedia>

class WavFileWriter final : public QFile
{
	Q_OBJECT
public:
	explicit WavFileWriter(const QString& filename,
	                       const QAudioFormat& format,
	                       QObject* parent = nullptr);
	~WavFileWriter() override;

	bool open();
	void close() override;

private:
	void writeHeader();
	bool hasSupportedFormat();

	QAudioFormat format_;
};

#endif // _WAV_FILE_WRITER_H_
