/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "AudioLevel.h"

#include <QDebug>
#include <QPainter>
#include <QtMath>

namespace SpeexWebRTCTest {

AudioLevel::AudioLevel(QWidget* parent) : QWidget(parent)
{
	setMinimumWidth(30);
	setMaximumWidth(30);
}

void AudioLevel::setLevel(qreal level)
{
	if (level_ != level)
	{
		level_ = level;
		update();
	}
}

void AudioLevel::paintEvent(QPaintEvent* event)
{
	Q_UNUSED(event);

	const float maxLevel = 0;
	const float minLevel = -50;
	const float redLevel = -1;
	const float yellowLevel = -6;

	const QColor greenColor = QColor(42, 168, 43);
	const QColor yellowColor = "yellow";
	const QColor redColor = "red";

	QPainter painter(this);

	auto drawArea = [&](float min, float max, const QColor& color)
	{
		int maxHeight = std::floor((max - minLevel) / (maxLevel - minLevel) * height());
		int minHeight = std::floor((min - minLevel) / (maxLevel - minLevel) * height());
		painter.fillRect(0, height() - maxHeight, width(), maxHeight - minHeight, color);
	};

	drawArea(minLevel, level_, greenColor);
	drawArea(yellowLevel, level_, yellowColor);
	drawArea(redLevel, level_, redColor);
	drawArea(level_, maxLevel, Qt::black);
}

} // namespace SpeexWebRTCTest
