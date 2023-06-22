#pragma once
#include <QWidget>
#include <QUndoStack>

class Graph final : public QWidget
{
    Q_OBJECT
public:
    static constexpr unsigned LIMIT_GRAPH_X = 1;
    static constexpr unsigned LIMIT_GRAPH_Y = 2;

    Graph(QWidget* parent = nullptr);
    ~Graph();

    void setInteractive(bool interactive);
    void undo();
    void redo();
    void fit();
    void setGraphFlags(unsigned flags);
    void setRange(double xMin, double xMax, double yMin, double yMax);
    void setPoints(const std::vector<QPointF>& points, bool undoable = true);
    const std::vector<QPointF>& points() const;
    void moveSelectedPoint(QPointF pt, bool undoable = true);
    void scaleAndTranslate(const QPointF& scale, const QPointF& translation);

    Q_SIGNAL void pointSelected(int i) const;
    Q_SIGNAL void pointMoved(size_t i) const;
    Q_SIGNAL void canUndoChanged(bool canUndo) const;
    Q_SIGNAL void canRedoChanged(bool canRedo) const;

private:
    class GraphAction;

    enum class Mode
    {
        Idle,
        MovePoint,
        PanView
    };

    void setPointsImpl(const std::vector<QPointF>& points);
    QPointF mapToViewport(const QPoint& p) const;
    QPoint mapFromViewport(const QPointF& p) const;
    void paintEvent(QPaintEvent*) override;
    void wheelEvent(QWheelEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mousePressEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void contextMenuEvent(QContextMenuEvent* e) override;
    QRect pointRect(size_t i) const;

private:
    bool m_interactive;
    QUndoStack m_undoStack;
    QRectF m_viewport;
    std::vector<QPointF> m_points;
    std::vector<QPointF> m_pointsBackup;
    int m_selected;
    Mode m_mode;
    QPoint m_panOrigin;
    QPointF m_moveClickOffset;
    unsigned m_flags;
};
