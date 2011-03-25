#ifndef WTSAUDIO_H
#define WTSAUDIO_H

#include "synced.h"

#include <portaudio.h>
#include <QObject>
#include <QLinkedList>

class SoundBuffer;

class WtsAudio : public QObject
{
    Q_OBJECT
public:
    class BufferAt : public WTS::Synced
    {
    public:
        BufferAt(QObject * parent = 0) : WTS::Synced(0, parent), m_buffer(0), m_playOffset(0) {}
        BufferAt(SoundBuffer * buffer, qint64 at, QObject * parent = 0) : WTS::Synced(at, parent), m_buffer(buffer), m_playOffset(0) {}

        SoundBuffer * buffer() { return m_buffer; }
        qint64 playOffset() const { return m_playOffset; }
        void setPlayOffset(qint64 offset) { m_playOffset = offset; }

    protected:
        SoundBuffer * m_buffer;
        qint64 m_playOffset;
    };

    static bool startsBefore( const BufferAt * a, const BufferAt * b ) { return *a < *b; }

    explicit WtsAudio(QObject *parent = 0);
    virtual ~WtsAudio();

    qint64 capture(SoundBuffer * buffer);
    qint64 currentSampleOffset() const;

    static qint64 samplingRate() { return 44100; }
    static int channelCount() { return 1; }
    static qint64 msToSampleCount(qint64 ms) { return samplingRate() * channelCount() * ms / 1000; }
    static qint64 sampleCountToMs(qint64 count) { return count * 1000 / (samplingRate() * channelCount()); }
    static qint64 byteToSampleCount(qint64 byteCount) { return byteCount / sizeof(float); }

    void samplerMix(qint64 ms, QVector<int16_t>& mix);

signals:

public slots:
    void start();
    void stop();

    void samplerClock(qint64 ms);
    void samplerSchedule(WtsAudio::BufferAt * buffer);
    void samplerClear();

protected:
    PaStream * m_stream;
    qint64 m_clock;
    QLinkedList< WtsAudio::BufferAt * > m_activeBuffers;
};

#endif // WTSAUDIO_H
