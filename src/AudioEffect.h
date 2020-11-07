#ifndef _AUDIO_EFFECT_H_
#define _AUDIO_EFFECT_H_

#include <QAudioBuffer>
#include <QDebug>
#include <QObject>
#include <QVariant>

namespace SpeexWebRTCTest {

class AudioEffect : public QObject
{
	Q_OBJECT
public:
	AudioEffect(unsigned int frameSize, const QAudioFormat& mainFormat, const QAudioFormat& auxFormat);

	virtual void processFrame(QAudioBuffer& mainBuffer, const QAudioBuffer& auxBuffer) = 0;

	virtual void setParameter(const QString& param, QVariant value) = 0;

	unsigned int getFrameSize() const;
	const QAudioFormat& getMainFormat() const;
	const QAudioFormat& getAuxFormat() const;

protected:
	void setVoiceActive(bool active)
	{
		if (voiceActive_ != active)
			emit voiceActivityChanged(active);
		voiceActive_ = active;
	}

signals:
	void voiceActivityChanged(bool voice);

private:
	const unsigned int frameSize_;
	const QAudioFormat mainFormat_;
	const QAudioFormat auxFormat_;

	bool voiceActive_ = false;
};

} // namespace SpeexWebRTCTest

#endif //_AUDIO_EFFECT_H_
