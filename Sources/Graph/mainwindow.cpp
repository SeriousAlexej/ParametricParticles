#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "scaletranslatedialog.h"
#include "./ui_scaletranslatedialog.h"

#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QClipboard>
#include <QMessageBox>
#include <QShortcut>
#include <QKeySequence>
#include <QMimeData>

namespace
{
constexpr char g_clipboardPrefix[] = "GraphClipboard ";
}

MainWindow::MainWindow(const QString& graphPath, QWidget* parent)
    : QMainWindow(parent)
    , ui(std::make_unique<Ui::MainWindow>())
    , m_graphPath(graphPath)
    , m_exitCode(42)
{
    ui->setupUi(this);

    QFile graphFile(m_graphPath);
    if (QFileInfo(graphFile).completeSuffix() == "txt" && graphFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QTextStream graphStream(&graphFile);
        const QString flags = graphStream.readLine();
        ui->graph->setGraphFlags(flags.toUInt());
        const QString title = graphStream.readLine();
        if (!title.isEmpty())
            setWindowTitle(title);
        std::vector<QPointF> points;
        while (!graphStream.atEnd())
        {
            QPointF point;
            graphStream >> point.rx();
            if (!graphStream.atEnd())
            {
                graphStream >> point.ry();
                points.push_back(point);
            }
        }
        ui->graph->setPoints(points, false);
        ui->graph->fit();
    }

    setWindowFlag(Qt::WindowMaximizeButtonHint, false);
    setWindowFlag(Qt::WindowMinimizeButtonHint, false);
    connect(ui->graph, &Graph::pointSelected, this, &MainWindow::onPointSelected);
    connect(ui->graph, &Graph::pointMoved, this, &MainWindow::updateFromPoint);
    connect(ui->posX, &QDoubleSpinBox::editingFinished, this, &MainWindow::updateFromSpinbox);
    connect(ui->posY, &QDoubleSpinBox::editingFinished, this, &MainWindow::updateFromSpinbox);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &MainWindow::saveGraphAndClose);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &MainWindow::close);
    connect(ui->btnCopy, &QPushButton::clicked, this, &MainWindow::onCopy);
    connect(ui->btnPaste, &QPushButton::clicked, this, &MainWindow::onPaste);
    connect(ui->btnScaleAndTranslate, &QPushButton::clicked, this, &MainWindow::onScaleAndTranslate);
    connect(ui->graph, &Graph::canUndoChanged, ui->btnUndo, &QPushButton::setEnabled);
    connect(ui->graph, &Graph::canRedoChanged, ui->btnRedo, &QPushButton::setEnabled);
    connect(ui->btnUndo, &QPushButton::clicked, ui->graph, &Graph::undo);
    connect(ui->btnRedo, &QPushButton::clicked, ui->graph, &Graph::redo);

    new QShortcut(QKeySequence::Undo, this, ui->btnUndo, &QPushButton::click);
    new QShortcut(QKeySequence::Redo, this, ui->btnRedo, &QPushButton::click);
}

MainWindow::~MainWindow()
{
}

int MainWindow::exitCode() const
{
    return m_exitCode;
}

void MainWindow::onPointSelected(int i)
{
    if (i < 0 || i >= static_cast<int>(ui->graph->points().size()))
    {
        ui->posX->setEnabled(false);
        ui->posY->setEnabled(false);
        return;
    }
    ui->posX->setEnabled(true);
    ui->posY->setEnabled(true);
    updateFromPoint(i);
}

void MainWindow::updateFromPoint(size_t i)
{
    QSignalBlocker blockX(ui->posX);
    QSignalBlocker blockY(ui->posY);
    ui->posX->setValue(ui->graph->points()[i].x());
    ui->posY->setValue(ui->graph->points()[i].y());
}

void MainWindow::updateFromSpinbox()
{
    if (!ui->posX->isEnabled() || !ui->posY->isEnabled())
        return;
    const QPointF pt(ui->posX->value(), ui->posY->value());
    ui->graph->moveSelectedPoint(pt);
}

void MainWindow::saveGraphAndClose()
{
    QFile graphFile(m_graphPath);
    if (graphFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QTextStream graphStream(&graphFile);
        for (const auto& point : ui->graph->points())
            graphStream << point.x() << ' ' << point.y() << ' ';
        m_exitCode = 0;
    }
    close();
}

void MainWindow::onCopy()
{
    QString graphString = g_clipboardPrefix;
    QTextStream graphStream(&graphString);
    for (const auto& point : ui->graph->points())
        graphStream << point.x() << ' ' << point.y() << ' ';
    QGuiApplication::clipboard()->setText(graphString);
}

void MainWindow::onPaste()
{
    auto* clipboard = QGuiApplication::clipboard()->mimeData();
    if (!clipboard->hasText() || !clipboard->text().startsWith(g_clipboardPrefix))
        return;
    QString graphString = clipboard->text();
    graphString = graphString.right(graphString.length() - static_cast<int>(strlen(g_clipboardPrefix)));
    QTextStream graphStream(&graphString);
    std::vector<QPointF> points;
    while (!graphStream.atEnd())
    {
        QPointF point;
        graphStream >> point.rx();
        if (!graphStream.atEnd())
        {
            graphStream >> point.ry();
            points.push_back(point);
        }
    }
    ui->graph->setPoints(points);
    ui->graph->fit();
}

void MainWindow::onScaleAndTranslate()
{
    ScaleTranslateDialog d(this);
    if (d.exec() == QDialog::Accepted)
    {
        const QPointF scale(d.ui->xScale->value(), d.ui->yScale->value());
        const QPointF translation(d.ui->xTranslation->value(), d.ui->yTranslation->value());
        ui->graph->scaleAndTranslate(scale, translation);
    }
}
