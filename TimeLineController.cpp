#include "TimeLineController.hpp"
#include "Project.h"
#include "SoundBuffer.h"

using namespace WTS;

TimeLineController::TimeLineController(QObject *parent) :
    QObject(parent)
{
}

void TimeLineController::setProject(Project *project)
{
    m_project = project;
}
void TimeLineController::seek(qint64 ms)
{
    start();
}

void TimeLineController::start()
{
    m_sequence = m_project->getSequence();
    m_sequenceCursor = beginCursor();
}

void TimeLineController::advanceSequenceCursor(qint64 ms)
{
    while( m_sequenceCursor != m_sequence.end()
            && ((*m_sequenceCursor)->at()
                + WtsAudio::sampleCountToMs((*m_sequenceCursor)->buffer()
                    ->rangeStart())) <= ms ) {
        emit samplerSchedule( *m_sequenceCursor );
        m_sequenceCursor++;
    }
}

QList<WtsAudio::BufferAt *>::iterator TimeLineController::beginCursor()
{
    qSort(m_sequence.begin(), m_sequence.end(), WtsAudio::startsBefore);
    return m_sequence.begin();
}

QList<WtsAudio::BufferAt *>::iterator TimeLineController::endCursor()
{
     return m_sequence.end();
}

