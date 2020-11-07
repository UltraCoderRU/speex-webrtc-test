#ifndef _MAIN_WINDOW_H_
#define _MAIN_WINDOW_H_

#include "AudioLevel.h"
#include "AudioProcessor.h"

#include <QAudioInput>
#include <QAudioOutput>
#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

namespace SpeexWebRTCTest {

class AudioProcessor;

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow();
	~MainWindow() override;

private slots:
	void changeDevicesConfiguration();
	void switchBackend();
	void changeNoiseReductionSettings();
	void changeAGCSettings();
	void changeAECSettings();

	void updateVoiceActivity(bool);
	void updateInputAudioLevels(const QVector<qreal>&);
	void updateOutputAudioLevels(const QVector<qreal>&);

private:
	void initializeAudio(const QAudioDeviceInfo& inputDeviceInfo,
	                     const QAudioDeviceInfo& outputDeviceInfo,
	                     const QAudioDeviceInfo& monitorDeviceInfo);

	void startRecording();
	void stopRecording();

	Backend currentBackend() const;

	void setupDials(Backend backend);

	Ui::MainWindow* ui = nullptr;

	QScopedPointer<QAudioInput> audioInput_;
	QScopedPointer<QAudioInput> monitorInput_;
	QScopedPointer<QAudioOutput> audioOutput_;

	QBuffer monitorBuffer_;

	QScopedPointer<AudioProcessor> processor_;

	QList<AudioLevel*> inputAudioLevels_;
	QList<AudioLevel*> outputAudioLevels_;
};

} // namespace SpeexWebRTCTest

#endif // _MAIN_WINDOW_H_
