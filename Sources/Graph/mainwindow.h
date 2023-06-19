#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <memory>

namespace Ui { class MainWindow; }

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(const QString& graphPath, QWidget* parent = nullptr);
    ~MainWindow();

    int exitCode() const;

private:
    void onPointSelected(int i);
    void updateFromPoint(size_t i);
    void updateFromSpinbox();
    void saveGraphAndClose();
    void onCopy();
    void onPaste();
    void onScaleAndTranslate();

private:
    std::unique_ptr<Ui::MainWindow> ui;
    const QString m_graphPath;
    int m_exitCode;
};
#endif // MAINWINDOW_H
