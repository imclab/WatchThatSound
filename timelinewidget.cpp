#include "timelinewidget.h"
#include "ui_mainwindow.h"
#include <QMouseEvent>

#include "synced.h"

TimeLineWidget::TimeLineWidget(QWidget *parent)
    : QGraphicsView(parent)
    , m_seekOnDrag(false)
    , m_currentTime(0)
    , m_editMode(true)
    , m_draggedItem(0)
{
    // find the mainWindow
    QObject * iter = parent;
    while(iter && !iter->inherits("MainWindow"))
        iter = iter->parent();
    if (!iter)
        qFatal("TimeLineWidget should be descendant of MainWindow in view hierarchy");
    m_mainWindow = qobject_cast<MainWindow *>(iter);

    connect(m_mainWindow,SIGNAL(storyBoardChanged()),SLOT(update()));
    connect(m_mainWindow,SIGNAL(samplerClock(qint64)),SLOT(setCurrentTime(qint64)));

    setScene(new QGraphicsScene(0.0,0.0,1.0,1.0,this));
}

void TimeLineWidget::resizeEvent ( QResizeEvent * /*event*/ )
{
    fitInView(0,0,1.0,1.0);
}

void TimeLineWidget::setCurrentTime(qint64 time)
{
    Phonon::MediaObject * mo = m_mainWindow->mediaObject();
    if (!mo) return;

    if (time != m_currentTime) {
        // erase old cursor
        qint64 x = m_currentTime * width() / mo->totalTime();
        update( (int) x - 3, 0, (int) x + 3, height() );

        m_currentTime = time;

        // draw new cursor
        x = m_currentTime * width() / mo->totalTime();
        update( (int)x - 1, 0, (int) x + 1, height() );
    }
}

void TimeLineWidget::paintRange(QPainter * painter, qreal x, qreal w, const QColor& c)
{
    QRectF r(x, 0, w, 1);
    painter->setPen(c);
    painter->drawRect( r );
    painter->fillRect( r, QBrush(c) );
}

void TimeLineWidget::drawBackground ( QPainter * painter, const QRectF & /*rect*/ )
{
    qreal total = m_mainWindow->mediaObject()->totalTime();
    qreal relX1 = 0.0f;

    QPainter::RenderHints oldHints = painter->renderHints();
    painter->setRenderHint(QPainter::Antialiasing, false);

    QColor colors[] = {
        QColor(170,255,170),
        QColor(200,255,200),
    };
    int currentColor = 0;

    foreach(MainWindow::Marker * marker, m_mainWindow->getMarkers(MainWindow::SCENE)) {
        qreal relX2 = (qreal)marker->at() / total;
        // draw a rectangle which as tall as a widget and runs from relX1 to relX2
        paintRange(painter, relX1, relX2-relX1, colors[currentColor]);
        currentColor = (currentColor+1) % 2;
        relX1 = relX2;
    }
    // the last one: form relX1 to 1.0
    paintRange(painter, relX1, 1.0f - relX1, colors[currentColor]);

    // draw tension curve antialised...
    //painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(QColor(0,0,0,200));
    foreach(MainWindow::Marker * marker, m_mainWindow->getMarkers(MainWindow::EVENT)) {
        qreal relX2 = (qreal)marker->at() / total;
        painter->drawLine(QPointF(relX2,0.), QPointF(relX2,1.));
    }

    painter->setPen(QColor(255,100,100,127));
    painter->drawPath( m_mainWindow->tensionCurve() );

    painter->setRenderHints(oldHints, true);
}

void TimeLineWidget::drawForeground ( QPainter * painter, const QRectF & rect )
{
    QPainter::RenderHints oldHints = painter->renderHints();
    painter->setRenderHint(QPainter::Antialiasing, false);

    float x = (float)m_currentTime / (float)m_mainWindow->mediaObject()->totalTime();

    if (x > rect.x() && x < rect.right()) {
        painter->setPen(QColor(0,0,255,100));
        painter->drawLine(QPointF(x,0),QPointF(x,1));
    }

    if (scene()->focusItem()) {
        painter->drawRect(scene()->focusItem()->sceneBoundingRect());
    }

    painter->setRenderHints(oldHints, true);
}

void TimeLineWidget::seekTo(qint64 x) {
    m_mainWindow->seek(m_mainWindow->mediaObject()->totalTime() * x / (qint64)width());
}

void TimeLineWidget::mousePressEvent ( QMouseEvent * event )
{
    QGraphicsView::mousePressEvent(event);

    // check if a draggable item was hit
    m_lastDragPos = mapToScene(event->pos());
    m_draggedItem  = itemAt( event->pos() );
    while(m_draggedItem) {
        m_draggedItemFlags = itemDragFlags(m_draggedItem);
        if (m_draggedItemFlags & DRAG_XY)
            break;
        m_draggedItem = m_draggedItem->parentItem();
    }
    if (seekOnDrag()) {
        if (event->buttons() & Qt::LeftButton) {
            seekTo(event->x());
        }
    }
}

void TimeLineWidget::mouseMoveEvent ( QMouseEvent * event )
{
    QGraphicsView::mouseMoveEvent(event);

    // do we have a dragged item?
    if (m_draggedItem) {
        QPointF newPos = mapToScene(event->pos());
        QPointF delta = newPos - m_lastDragPos;
        m_lastDragPos = newPos;

        if (m_draggedItemFlags & DRAG_X)
            m_draggedItem->setX( m_draggedItem->x() + delta.x() );
        if (m_draggedItemFlags & DRAG_Y)
            m_draggedItem->setY( m_draggedItem->y() + delta.y() );
    }

    if (seekOnDrag()) {
        if (event->buttons() & Qt::LeftButton) {
            seekTo(event->x());
        }
    }
}

void TimeLineWidget::mouseReleaseEvent(QMouseEvent *event)
{
    QGraphicsView::mouseReleaseEvent(event);

    if (m_draggedItem)
        m_draggedItem = 0;
}

void TimeLineWidget::assignSynced(QGraphicsItem *item, WTS::Synced *synced)
{
    item->setData(SYNCED, QVariant::fromValue(synced));
}

QGraphicsItem * TimeLineWidget::findSynced(QGraphicsItem *item, WTS::Synced **synced)
{
    while( item && !item->data(SYNCED) .isValid() ) {
        item = item->parentItem();
    }
    if (item) {
        *synced = item->data(SYNCED).value<WTS::Synced*>();
        return item;
    } else {
        *synced = 0;
        return 0;
    }
}

void TimeLineWidget::setItemDragFlags(QGraphicsItem *item, int options)
{
    item->setData(DRAG_OPTIONS, options);
}

int TimeLineWidget::itemDragFlags(QGraphicsItem * item)
{
    return item->data(DRAG_OPTIONS).toInt();
}

