#include "scaletranslatedialog.h"
#include "ui_scaletranslatedialog.h"

ScaleTranslateDialog::ScaleTranslateDialog(QWidget *parent) :
    QDialog(parent),
    ui(std::make_unique<Ui::ScaleTranslateDialog>())
{
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    ui->setupUi(this);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &ScaleTranslateDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &ScaleTranslateDialog::reject);
}

ScaleTranslateDialog::~ScaleTranslateDialog()
{
}
