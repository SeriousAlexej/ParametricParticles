#include "graph.h"
#include <QPainter>
#include <QWheelEvent>
#include <QMenu>
#include <algorithm>
#include <QPointer>

class Graph::GraphAction final : public QUndoCommand
{
public:
    GraphAction(Graph& graph, const std::vector<QPointF>& newPoints)
        : mp_graph(&graph)
        , m_oldPoints(graph.points())
        , m_newPoints(newPoints)
    {
    }

    void undo() final
    {
        if (mp_graph)
            mp_graph->setPointsImpl(m_oldPoints);
    }

    void redo() final
    {
        if (mp_graph)
            mp_graph->setPointsImpl(m_newPoints);
    }

private:
    QPointer<Graph> mp_graph;
    std::vector<QPointF> m_oldPoints;
    std::vector<QPointF> m_newPoints;
};

Graph::Graph(QWidget* parent)
 : QWidget(parent)
{
    setInteractive(true);
    m_undoStack.setUndoLimit(100);
    setRange(0, 1, 0, 1);
    setPoints({ {0.0, 1.0} }, false);
    m_selected = -1;
    m_mode = Mode::Idle;
    m_flags = 0;

    connect(&m_undoStack, &QUndoStack::canUndoChanged, this, &Graph::canUndoChanged);
    connect(&m_undoStack, &QUndoStack::canRedoChanged, this, &Graph::canRedoChanged);
}

Graph::~Graph()
{
}

void Graph::setInteractive(bool interactive)
{
    m_interactive = interactive;
    setMouseTracking(m_interactive);
}

void Graph::undo()
{
    if (m_mode == Mode::Idle)
        m_undoStack.undo();
}

void Graph::redo()
{
    if (m_mode == Mode::Idle)
        m_undoStack.redo();
}

void Graph::fit()
{
    if (m_points.size() < 2)
        return;

    double minY = m_points.front().y();
    double maxY = minY;
    for (const auto& pt : m_points)
    {
        minY = std::min(minY, pt.y());
        maxY = std::max(maxY, pt.y());
    }
    const auto height = maxY - minY;
    const auto minX = m_points.front().x();
    const auto maxX = m_points.back().x();
    const auto width = maxX - minX;
    const auto delta = std::max(width, height);

    if (delta > 1.0f)
        setRange((minX + maxX)*0.5 - delta*0.5, (minX + maxX)*0.5 + delta*0.5, (minY + maxY)*0.5 - delta*0.5, (minY + maxY)*0.5 + delta*0.5);
}

void Graph::setGraphFlags(unsigned flags)
{
    m_flags = flags;
    update();
}

void Graph::setRange(double xMin, double xMax, double yMin, double yMax)
{
    m_viewport = QRectF(xMin-0.1, yMax+0.1, xMax-xMin+0.2, yMax-yMin+0.2);
}

void Graph::setPoints(const std::vector<QPointF>& points, bool undoable)
{
    if (m_points == points)
        return;

    auto action = std::make_unique<GraphAction>(*this, points);
    if (undoable)
        m_undoStack.push(action.release());
    else
        action->redo();
}

const std::vector<QPointF>& Graph::points() const
{
    return m_points;
}

void Graph::moveSelectedPoint(QPointF pt, bool undoable)
{
    if (m_selected < 0 || m_selected >= static_cast<int>(m_points.size()))
        return;

    auto newPoints = m_points;
    if (m_selected > 0)
        pt.rx() = std::max(pt.x(), newPoints[m_selected - 1].x() + 0.001);
    if (m_selected < static_cast<int>(newPoints.size() - 1))
        pt.rx() = std::min(pt.x(), newPoints[m_selected + 1].x() - 0.001);
    newPoints[m_selected] = pt;
    setPoints(newPoints, undoable);
}

void Graph::scaleAndTranslate(const QPointF& scale, const QPointF& translation)
{
    auto newPoints = m_points;
    for (auto& pt : newPoints)
    {
        pt.rx() *= scale.x();
        pt.ry() *= scale.y();
        pt += translation;
    }
    setPoints(newPoints);
}

void Graph::setPointsImpl(const std::vector<QPointF>& points)
{
    m_points = points;
    if (m_points.empty())
        m_points = { {0.0, 1.0} };
    std::stable_sort(m_points.begin(), m_points.end(), [](const QPointF& lhs, const QPointF& rhs)
    {
        return lhs.x() < rhs.x();
    });

    if (m_selected >= 0 && m_selected < static_cast<int>(m_points.size()))
    {
        pointMoved(m_selected);
    }
    else
    {
        m_selected = -1;
        pointSelected(m_selected);
    }

    update();
}

QPointF Graph::mapToViewport(const QPoint& p) const
{
    const double x = static_cast<double>(p.x()) / rect().width();
    const double y = static_cast<double>(p.y()) / rect().height();
    return { m_viewport.left() + m_viewport.width() * x, m_viewport.top() - m_viewport.height() * y };
}

QPoint Graph::mapFromViewport(const QPointF& p) const
{
    const double x = (p.x() - m_viewport.left()) / m_viewport.width();
    const double y = -(p.y() - m_viewport.top()) / m_viewport.height();
    return { static_cast<int>(rect().width() * x), static_cast<int>(rect().height() * y) };
}

void Graph::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    if (!p.isActive())
        return;

    // background
    const auto& r = rect();
    p.setRenderHint(QPainter::Antialiasing, false);
    p.fillRect(r, Qt::white);

    // grid
    p.setPen(QColor::fromRgb(223, 223, 223));
    const auto referencePoint = mapToViewport({50, 50});
    const double gridXstep = std::pow(2.0, std::round(std::log2(std::abs(referencePoint.x() - m_viewport.x()))));
    const double gridYstep = std::pow(2.0, std::round(std::log2(std::abs(referencePoint.y() - m_viewport.y()))));
    std::vector<std::pair<int, double>> verticalStops;
    std::vector<std::pair<int, double>> horizontalStops;
    for (double x = std::round(m_viewport.x() / gridXstep) * gridXstep;
         x < m_viewport.x() + m_viewport.width();
         x += gridXstep)
    {
        const auto pt = mapFromViewport({x, 0});
        p.drawLine(pt.x(), 0, pt.x(), r.height());
        verticalStops.emplace_back(std::make_pair(pt.x(), x));
    }
    for (double y = std::round(m_viewport.y() / gridYstep) * gridYstep;
         y > m_viewport.y() - m_viewport.height();
         y -= gridYstep)
    {
        const auto pt = mapFromViewport({0, y});
        p.drawLine(0, pt.y(), r.width(), pt.y());
        horizontalStops.emplace_back(std::make_pair(pt.y(), y));
    }

    // axes
    const auto& fontMetrics = p.fontMetrics();
    const auto zero = mapFromViewport({0, 0});
    const auto one = mapFromViewport({1, 1});
    const QBrush forbiddenArea(QColor::fromRgb(31, 31, 31, 60));
    p.setPen(QColor::fromRgb(31, 31, 31));
    p.drawLine(zero.x(), 0, zero.x(), r.height());
    p.fillRect(0, 0, zero.x(), r.height(), forbiddenArea);
    QString xAxisLabel = "seconds";
    if (m_flags & LIMIT_GRAPH_X)
    {
        p.fillRect(one.x(), 0, r.width() - one.x(), r.height(), forbiddenArea);
        xAxisLabel = "relative time";
    }
    if (m_flags & LIMIT_GRAPH_Y)
    {
        if (m_flags & LIMIT_GRAPH_X)
        {
            p.fillRect(zero.x(), 0, one.x() - zero.x(), one.y(), forbiddenArea);
            p.fillRect(zero.x(), zero.y(), one.x() - zero.x(), r.height() - zero.y(), forbiddenArea);
        } else {
            p.fillRect(zero.x(), 0, r.width() - zero.x(), one.y(), forbiddenArea);
            p.fillRect(zero.x(), zero.y(), r.width() - zero.x(), r.height() - zero.y(), forbiddenArea);
        }
    }
    p.drawLine(0, zero.y(), r.width(), zero.y());
    const auto xAxisLabelRect = fontMetrics.boundingRect(xAxisLabel);
    p.drawText(r.width() - xAxisLabelRect.width(), zero.y() + xAxisLabelRect.height(), xAxisLabel);

    // grid labels
    for (const auto& [x, value] : verticalStops)
    {
        const auto s = QString::number(value);
        const auto sRect = fontMetrics.boundingRect(s);
        p.drawText(x - sRect.width() / 2, r.height(), s);
    }
    for (const auto& [y, value] : horizontalStops)
    {
        const auto s = QString::number(value);
        const auto sRect = fontMetrics.boundingRect(s);
        p.drawText(0, y + sRect.height() / 2, s);
    }

    if (m_points.empty())
        return;
    // points left tail
    p.setPen(QColor::fromRgb(0, 0, 255));
    auto pt = mapFromViewport(m_points.front());
    p.drawLine(0, pt.y(), pt.x(), pt.y());
    // points right tail
    pt = mapFromViewport(m_points.back());
    p.drawLine(pt.x(), pt.y(), r.width(), pt.y());
    // points segments
    for (size_t i = 1; i < m_points.size(); ++i)
    {
        const auto p0 = mapFromViewport(m_points[i - 1]);
        const auto p1 = mapFromViewport(m_points[i]);
        p.drawLine(p0, p1);
    }
    // points
    for (size_t i = 0; i < m_points.size(); ++i)
    {
        const auto pt = mapFromViewport(m_points[i]);
        p.fillRect(pt.x() - 2, pt.y() - 2, 6, 6, m_selected == static_cast<int>(i) ? Qt::red : Qt::black);
    }
}

void Graph::wheelEvent(QWheelEvent* e)
{
    if (!m_interactive)
        return;

    if (m_mode != Mode::Idle)
        return;
    const int delta = e->angleDelta().y();
    if (delta == 0)
        return;

    const auto coord = mapToViewport(e->position().toPoint());
    const auto x = (m_viewport.left() - coord.x()) / m_viewport.width();
    const auto y = (m_viewport.top() - coord.y()) / m_viewport.height();

    double scale = 1.0 + delta * 0.001;
    m_viewport.setWidth(m_viewport.width() * scale);
    m_viewport.setHeight(m_viewport.height() * scale);
    m_viewport.moveTopLeft({coord.x() + x * m_viewport.width(), coord.y() + y * m_viewport.height()});
    update();
}

void Graph::mouseMoveEvent(QMouseEvent* e)
{
    if (!m_interactive)
        return;

    switch (m_mode)
    {
    case Mode::Idle:
    {
        bool hasPointUnderCursor = false;
        for (size_t i = 0; i < m_points.size(); ++i)
        {
            if (pointRect(i).contains(e->pos(), true))
            {
                hasPointUnderCursor = true;
                setCursor(Qt::SizeAllCursor);
                break;
            }
        }
        if (!hasPointUnderCursor)
            setCursor(Qt::ArrowCursor);
        break;
    }
    case Mode::MovePoint:
    {
        auto pt = mapToViewport(e->pos()) + m_moveClickOffset;
        moveSelectedPoint(pt, false);
        break;
    }
    case Mode::PanView:
    {
        const auto ptDelta = e->pos() - m_panOrigin;
        const auto& r = rect();
        m_viewport.moveTopLeft(m_moveClickOffset + QPointF
        { static_cast<double>(-ptDelta.x()) / r.width() * m_viewport.width(),
          static_cast<double>(ptDelta.y()) / r.height() * m_viewport.height() });
        update();
        break;
    }
    default:
        break;
    }
}

void Graph::mousePressEvent(QMouseEvent* e)
{
    if (!m_interactive)
        return;

    switch (m_mode)
    {
    case Mode::Idle:
    {
        if (e->button() == Qt::MiddleButton)
        {
            setCursor(Qt::ClosedHandCursor);
            m_mode = Mode::PanView;
            m_moveClickOffset = m_viewport.topLeft();
            m_panOrigin = e->pos();
            break;
        }
        if (e->button() == Qt::LeftButton)
        {
            for (size_t i = 0; i < m_points.size(); ++i)
            {
                if (pointRect(i).contains(e->pos(), true))
                {
                    const auto shouldUpdate = m_selected != static_cast<int>(i);
                    m_mode = Mode::MovePoint;
                    m_pointsBackup = m_points;
                    m_selected = static_cast<int>(i);
                    m_moveClickOffset = m_points[i] - mapToViewport(e->pos());
                    if (shouldUpdate)
                    {
                        pointSelected(m_selected);
                        update();
                    }
                    return;
                }
            }
        }
        break;
    }
    default:
        break;
    }
}

void Graph::mouseReleaseEvent(QMouseEvent*)
{
    if (!m_interactive)
        return;

    if (m_mode == Mode::MovePoint)
    {
        const auto newPoints = m_points;
        m_points = m_pointsBackup;
        setPoints(newPoints);
    }
    m_mode = Mode::Idle;
}

void Graph::contextMenuEvent(QContextMenuEvent* e)
{
    if (!m_interactive)
        return;

    if (m_mode != Mode::Idle)
        return;

    int underCursor = -1;
    for (size_t i = 0; i < m_points.size(); ++i)
    {
        if (pointRect(i).contains(e->pos(), true))
        {
            underCursor = static_cast<int>(i);
            break;
        }
    }
    if (underCursor == -1)
    {
        QMenu menu;
        const auto* createAction = menu.addAction("Add point");
        if (menu.exec(e->globalPos()) == createAction)
        {
            auto newPoints = m_points;
            newPoints.push_back(mapToViewport(e->pos()));
            setPoints(newPoints);
        }
    } else {
        if (m_points.size() <= 1)
            return;

        QMenu menu;
        const auto* deleteAction = menu.addAction("Delete point");
        const auto* deleteButThisAction = menu.addAction("Delete all points except this");
        const auto* actionToExecute = menu.exec(e->globalPos());
        if (actionToExecute == deleteAction)
        {
            auto newPoints = m_points;
            newPoints.erase(newPoints.begin() + underCursor);
            setPoints(newPoints);
        }
        else if (actionToExecute == deleteButThisAction)
        {
            setPoints({m_points[underCursor]});
        }
    }
}

QRect Graph::pointRect(size_t i) const
{
    if (i >= m_points.size())
        return {};
    const auto pt = mapFromViewport(m_points[i]);
    return {pt.x() - 10, pt.y() - 10, 20, 20};
}
