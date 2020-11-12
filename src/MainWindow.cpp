#include "MainWindow.h"

#include "AudioLevel.h"
#include "ui_MainWindow.h"

#include <QLoggingCategory>

namespace SpeexWebRTCTest {

namespace {
Q_LOGGING_CATEGORY(Gui, "gui")
}

QAudioFormat getCaptureFormat()
{
	QAudioFormat format;
	format.setSampleRate(48000);
	format.setChannelCount(1);
	format.setSampleSize(16);
	format.setCodec("audio/pcm");
	format.setByteOrder(QAudioFormat::LittleEndian);
	format.setSampleType(QAudioFormat::SignedInt);
	return format;
}

QAudioFormat getOutputFormat()
{
	return getCaptureFormat();
}

QAudioFormat getMonitorFormat()
{
	QAudioFormat format;
	format.setSampleRate(48000);
	format.setChannelCount(2);
	format.setSampleSize(16);
	format.setCodec("audio/pcm");
	format.setByteOrder(QAudioFormat::LittleEndian);
	format.setSampleType(QAudioFormat::SignedInt);
	return format;
}

MainWindow::MainWindow() : ui(new Ui::MainWindow)
{
	ui->setupUi(this);

	audioInputThread_.start();
	audioOutputThread_.start();

	qDebug(Gui) << "Enumerating audio devices...";
	for (auto& deviceInfo : QAudioDeviceInfo::availableDevices(QAudio::AudioInput))
	{
		ui->inputDeviceSelector->addItem(deviceInfo.deviceName(), QVariant::fromValue(deviceInfo));
		ui->monitorDeviceSelector->addItem(deviceInfo.deviceName(), QVariant::fromValue(deviceInfo));
	}

	for (auto& deviceInfo : QAudioDeviceInfo::availableDevices(QAudio::AudioOutput))
		ui->outputDeviceSelector->addItem(deviceInfo.deviceName(), QVariant::fromValue(deviceInfo));

	connect(ui->inputDeviceSelector, QOverload<int>::of(&QComboBox::activated), this,
	        &MainWindow::changeDevicesConfiguration);
	connect(ui->outputDeviceSelector, QOverload<int>::of(&QComboBox::activated), this,
	        &MainWindow::changeDevicesConfiguration);

	connect(ui->speexRadioButton, &QRadioButton::toggled, this, &MainWindow::switchBackend);

	connect(ui->noiseGroupBox, &QGroupBox::toggled, this, &MainWindow::changeNoiseReductionSettings);
	connect(ui->noiseSuppressionDial, &QDial::valueChanged, this,
	        &MainWindow::changeNoiseReductionSettings);

	connect(ui->agcGroupBox, &QGroupBox::toggled, this, &MainWindow::changeAGCSettings);
	connect(ui->agcLevelDial, &QDial::valueChanged, this, &MainWindow::changeAGCSettings);
	connect(ui->agcMaxGainDial, &QDial::valueChanged, this, &MainWindow::changeAGCSettings);
	connect(ui->agcMaxIncrementDial, &QDial::valueChanged, this, &MainWindow::changeAGCSettings);
	connect(ui->agcMaxDecrementDial, &QDial::valueChanged, this, &MainWindow::changeAGCSettings);

	connect(ui->aecGroupBox, &QGroupBox::toggled, this, &MainWindow::changeAECSettings);
	connect(ui->aecSuppressionDial, &QDial::valueChanged, this, &MainWindow::changeAECSettings);

	initializeAudio(QAudioDeviceInfo::defaultInputDevice(), QAudioDeviceInfo::defaultOutputDevice(),
	                QAudioDeviceInfo::defaultInputDevice());

	startRecording();
}

MainWindow::~MainWindow()
{
	stopRecording();
	audioInputThread_.exit(0);
	audioOutputThread_.exit(0);
}

void fixFormatForDevice(QAudioFormat& format, const QAudioDeviceInfo& info)
{
	if (!info.isFormatSupported(format))
	{
		QAudioFormat newFormat = info.nearestFormat(format);
		qWarning(Gui).nospace() << "Preferred format " << format << " is not supported by device "
		                        << info.deviceName() << ".";
		qWarning(Gui) << "Trying to use nearest format" << newFormat;
		format = newFormat;
	}
}

void MainWindow::initializeAudio(const QAudioDeviceInfo& inputDeviceInfo,
                                 const QAudioDeviceInfo& outputDeviceInfo,
                                 const QAudioDeviceInfo& monitorDeviceInfo)
{
	qDebug(Gui) << "Initializing audio processing tract...";
	auto captureFormat = getCaptureFormat();
	auto outputFormat = getOutputFormat();
	auto monitorFormat = getMonitorFormat();

	fixFormatForDevice(captureFormat, inputDeviceInfo);
	fixFormatForDevice(outputFormat, outputDeviceInfo);
	fixFormatForDevice(monitorFormat, monitorDeviceInfo);

	audioInput_.reset(new QAudioInput(inputDeviceInfo, captureFormat));
	audioOutput_.reset(new QAudioOutput(outputDeviceInfo, outputFormat));
	monitorInput_.reset(new QAudioInput(monitorDeviceInfo, monitorFormat));

	audioInput_->moveToThread(&audioInputThread_);
	audioOutput_->moveToThread(&audioOutputThread_);
	monitorInput_->moveToThread(&audioInputThread_);

	processor_.reset(new AudioProcessor(captureFormat, monitorFormat, monitorBuffer_));
	connect(processor_.get(), &AudioProcessor::voiceActivityChanged, this,
	        &MainWindow::updateVoiceActivity);
	connect(processor_.get(), &AudioProcessor::inputLevelsChanged, this,
	        &MainWindow::updateInputAudioLevels);
	connect(processor_.get(), &AudioProcessor::outputLevelsChanged, this,
	        &MainWindow::updateOutputAudioLevels);
}

void MainWindow::startRecording()
{
	qDebug(Gui) << "Starting audio processing...";
	processor_->open(QIODevice::ReadWrite | QIODevice::Truncate);
	monitorBuffer_.open(QIODevice::ReadWrite | QIODevice::Truncate);

	audioInput_->start(processor_.get());
	audioOutput_->start(processor_.get());
	monitorInput_->start(&monitorBuffer_);

	qInfo(Gui) << "input buffer size:" << audioInput_->bufferSize();
	qInfo(Gui) << "output buffer size:" << audioOutput_->bufferSize();
}

void MainWindow::stopRecording()
{
	qDebug(Gui) << "Stopping audio processing...";
	if (monitorInput_)
		monitorInput_->stop();

	if (audioInput_)
		audioInput_->stop();

	if (audioOutput_)
		audioOutput_->stop();

	monitorBuffer_.close();

	if (processor_)
		processor_->close();
}

void MainWindow::changeDevicesConfiguration()
{
	stopRecording();
	initializeAudio(ui->inputDeviceSelector->itemData(ui->inputDeviceSelector->currentIndex())
	                    .value<QAudioDeviceInfo>(),
	                ui->outputDeviceSelector->itemData(ui->outputDeviceSelector->currentIndex())
	                    .value<QAudioDeviceInfo>(),
	                ui->monitorDeviceSelector->itemData(ui->monitorDeviceSelector->currentIndex())
	                    .value<QAudioDeviceInfo>());
}

void MainWindow::switchBackend()
{
	Backend newBackend = ui->speexRadioButton->isChecked() ? Backend::Speex : Backend::WebRTC;
	qInfo(Gui) << "Switching DSP backend...";
	setupDials(newBackend);
	processor_->switchBackend(newBackend);
}

QString levelFromCode(int value)
{
	switch (value)
	{
	case 0:
		return "Low";
	case 1:
		return "Moderate";
	case 2:
		return "High";
	case 3:
		return "Very High";
	default:
		throw std::invalid_argument("Invalid code for level");
	}
}

void MainWindow::setupDials(Backend backend)
{
	ui->noiseGroupBox->setChecked(false);
	ui->noiseSuppressionDial->setValue(0);

	ui->agcGroupBox->setChecked(false);
	ui->agcLevelDial->setValue(0);
	ui->agcLevelValue->setText("0 dBFS");
	ui->agcMaxGainDial->setValue(0);
	ui->agcMaxGainValue->setText("0 dB");
	ui->agcMaxIncrementDial->setValue(0);
	ui->agcMaxIncrementValue->setText("0 dB/sec");
	ui->agcMaxDecrementDial->setValue(0);
	ui->agcMaxDecrementValue->setText("0 dB/sec");

	ui->aecGroupBox->setChecked(false);
	ui->aecSuppressionDial->setValue(0);

	updateVoiceActivity(false);

	if (backend == Backend::Speex)
	{
		ui->noiseSuppressionDial->setMaximum(60);
		ui->noiseSuppressionValue->setText("0 dBFS");

		ui->agcMaxIncrementLabel->setVisible(true);
		ui->agcMaxIncrementDial->setVisible(true);
		ui->agcMaxIncrementValue->setVisible(true);

		ui->agcMaxDecrementLabel->setVisible(true);
		ui->agcMaxDecrementDial->setVisible(true);
		ui->agcMaxDecrementValue->setVisible(true);

		ui->aecSuppressionDial->setMaximum(60);
		ui->aecSuppressionValue->setText("0 dB");
	}
	else
	{
		ui->noiseSuppressionDial->setMaximum(3);
		ui->noiseSuppressionValue->setText(levelFromCode(0));

		ui->agcMaxIncrementLabel->setVisible(false);
		ui->agcMaxIncrementDial->setVisible(false);
		ui->agcMaxIncrementValue->setVisible(false);

		ui->agcMaxDecrementLabel->setVisible(false);
		ui->agcMaxDecrementDial->setVisible(false);
		ui->agcMaxDecrementValue->setVisible(false);

		ui->aecSuppressionDial->setMaximum(2);
		ui->aecSuppressionValue->setText(levelFromCode(0));
	}
}

void MainWindow::changeNoiseReductionSettings()
{
	if (currentBackend() == Backend::Speex)
	{
		std::int32_t enabled = ui->noiseGroupBox->isChecked() ? 1 : 0;
		processor_->setEffectParam("noise_reduction_enabled", enabled);

		std::int32_t maxAttenuation = -ui->noiseSuppressionDial->value();
		ui->noiseSuppressionValue->setText(QString("%1 dB").arg(-maxAttenuation));
		processor_->setEffectParam("noise_reduction_max_attenuation", maxAttenuation);
	}
	else
	{
		processor_->setEffectParam("noise_reduction_enabled", ui->noiseGroupBox->isChecked());
		int suppressionLevel = ui->noiseSuppressionDial->value();
		ui->noiseSuppressionValue->setText(levelFromCode(suppressionLevel));
		processor_->setEffectParam("noise_reduction_suppression_level", suppressionLevel);
	}
}

void MainWindow::changeAGCSettings()
{
	if (currentBackend() == Backend::Speex)
	{
		std::int32_t enabled = ui->agcGroupBox->isChecked() ? 1 : 0;
		processor_->setEffectParam("gain_control_enabled", enabled);

		std::int32_t level =
		    QAudio::convertVolume(-ui->agcLevelDial->value(), QAudio::DecibelVolumeScale,
		                          QAudio::LinearVolumeScale) *
		    32768;
		ui->agcLevelValue->setText(QString("%1 dBFS").arg(-ui->agcLevelDial->value()));
		processor_->setEffectParam("gain_control_level", level);

		std::int32_t maxGain = ui->agcMaxGainDial->value();
		ui->agcMaxGainValue->setText(QString("%1 dB").arg(maxGain));
		processor_->setEffectParam("gain_control_max_gain", maxGain);

		std::int32_t maxIncrement = ui->agcMaxIncrementDial->value();
		ui->agcMaxIncrementValue->setText(QString("%1 dB/sec").arg(maxIncrement));
		processor_->setEffectParam("gain_control_max_increment", maxIncrement);

		std::int32_t maxDecrement = ui->agcMaxDecrementDial->value();
		ui->agcMaxDecrementValue->setText(QString("%1 dB/sec").arg(maxDecrement));
		processor_->setEffectParam("gain_control_max_decrement", maxDecrement);
	}
	else
	{
		processor_->setEffectParam("gain_control_enabled", ui->agcGroupBox->isChecked());

		int level = ui->agcLevelDial->value();
		ui->agcLevelValue->setText(QString("%1 dBFS").arg(-ui->agcLevelDial->value()));
		processor_->setEffectParam("gain_control_target_level", level);

		int maxGain = ui->agcMaxGainDial->value();
		ui->agcMaxGainValue->setText(QString("%1 dB").arg(maxGain));
		processor_->setEffectParam("gain_control_max_gain", maxGain);
	}
}

void MainWindow::changeAECSettings()
{
	if (currentBackend() == Backend::Speex)
	{
		std::int32_t enabled = ui->aecGroupBox->isChecked() ? 1 : 0;
		processor_->setEffectParam("echo_cancellation_enabled", enabled);

		std::int32_t maxAttenuation = -ui->aecSuppressionDial->value();
		ui->aecSuppressionValue->setText(QString("%1 dB").arg(-maxAttenuation));

		processor_->setEffectParam("echo_cancellation_max_attenuation", maxAttenuation);
	}
	else
	{
		processor_->setEffectParam("echo_cancellation_enabled", ui->aecGroupBox->isChecked());

		int suppressionLevel = ui->aecSuppressionDial->value();
		ui->aecSuppressionValue->setText(levelFromCode(suppressionLevel));

		processor_->setEffectParam("echo_cancellation_suppression_level", suppressionLevel);
	}
}

Backend MainWindow::currentBackend() const
{
	return processor_->getCurrentBackend();
}

void MainWindow::updateVoiceActivity(bool active)
{
	if (active)
	{
		ui->vadLabel->setText("Voice active");
		ui->vadLabel->setStyleSheet("QLabel { color : #2aa82b; }");
	}
	else
	{
		ui->vadLabel->setText("Voice inactive");
		ui->vadLabel->setStyleSheet("QLabel { color : black; }");
	}
}

void MainWindow::updateInputAudioLevels(const QVector<qreal>& levels)
{
	if (inputAudioLevels_.count() != levels.size())
	{
		qDeleteAll(inputAudioLevels_);
		inputAudioLevels_.clear();
		for (int i = 0; i < levels.size(); ++i)
		{
			auto* level = new AudioLevel(ui->centralWidget);
			inputAudioLevels_.append(level);
			ui->inputLevelsLayout->addWidget(level);
		}
	}

	for (int i = 0; i < levels.count(); ++i)
		inputAudioLevels_.at(i)->setLevel(levels.at(i));
}

void MainWindow::updateOutputAudioLevels(const QVector<qreal>& levels)
{
	if (outputAudioLevels_.count() != levels.size())
	{
		qDeleteAll(outputAudioLevels_);
		outputAudioLevels_.clear();
		for (int i = 0; i < levels.size(); ++i)
		{
			auto* level = new AudioLevel(ui->centralWidget);
			outputAudioLevels_.append(level);
			ui->outputLevelsLayout->addWidget(level);
		}
	}

	for (int i = 0; i < levels.count(); ++i)
		outputAudioLevels_.at(i)->setLevel(levels.at(i));
}

} // namespace SpeexWebRTCTest
