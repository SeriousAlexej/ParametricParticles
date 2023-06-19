#ifndef SCALETRANSLATEDIALOG_H
#define SCALETRANSLATEDIALOG_H

#include <QDialog>

namespace Ui {
class ScaleTranslateDialog;
}

class ScaleTranslateDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ScaleTranslateDialog(QWidget* parent = nullptr);
    ~ScaleTranslateDialog();

public:
    std::unique_ptr<Ui::ScaleTranslateDialog> ui;
};

#endif // SCALETRANSLATEDIALOG_H
