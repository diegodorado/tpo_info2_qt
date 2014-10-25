/****************************************************************************
**
** Copyright (C) 2012 Denis Shienkov <denis.shienkov@gmail.com>
** Copyright (C) 2012 Laszlo Papp <lpapp@kde.org>
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtSerialPort module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QApplication>

#include "mainwindow.h"


#include <QFileDialog>
#include <QDebug>
#include "wav.h"


int main(int argc, char *argv[])
{
    //QApplication a(argc, argv);
    //MainWindow w;
    //w.show();
    //return a.exec();

    QString fileName = "/home/diego/music/8bit.wav"; // QFileDialog::getOpenFileName( this,tr("Open Image"), "", tr("WAV Files (*.wav)"));
    QFile file(fileName);

    wav_hdr_t *wav_hdr = new wav_hdr_t();
    if (file.open(QIODevice::ReadOnly)) {
        qint64 bytes = file.read((char *) wav_hdr, sizeof(wav_hdr_t));
        if (bytes == sizeof(wav_hdr_t)) {
            qDebug() << "read was good";
            qDebug() << "file.size() " << file.size();
            qDebug() << "fileSize : " << wav_hdr->fileSize();
            qDebug() << "dataSize : " << wav_hdr->dataSize();
            qDebug() << "WAV_HDR_CHUNK_ID : " << WAV_HDR_CHUNK_ID;
            qDebug() << "ChunkID : " << wav_hdr->ChunkID;
            qDebug() << "validChunkID : " << wav_hdr->validChunkID();
            qDebug() << "validFormat : " << wav_hdr->validFormat();
            qDebug() << "validSubchunk1ID : " << wav_hdr->validSubchunk1ID();
            qDebug() << "validSubchunk2ID : " << wav_hdr->validSubchunk2ID();
            qDebug() << "isPCM : " << wav_hdr->isPCM();
            qDebug() << "is8bit : " << wav_hdr->is8bit();
            qDebug() << "isMono : " << wav_hdr->isMono();
            qDebug() << "valid : " << wav_hdr->valid();



            int i = 7;

            // pointer to integer and back
            unsigned int v1 = reinterpret_cast<unsigned int>(&i); // static_cast is an error
            qDebug() << "The value of &i is 0x" << v1 << "\n";
            int* p1 = reinterpret_cast<int*>(v1);
            qDebug() <<  (p1 == &i);


            // type aliasing through pointer
            char* p2 = reinterpret_cast<char*>(&i);
            if(p2[0] == '\x7')
                qDebug() << "This system is little-endian\n";
            else
                qDebug() << "This system is big-endian\n";

        }
    }

}
