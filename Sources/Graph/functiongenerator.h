#ifndef FUNCTIONGENERATOR_H
#define FUNCTIONGENERATOR_H

#include <QDialog>

namespace Ui {
class FunctionGenerator;
}

class FunctionGenerator : public QDialog
{
    Q_OBJECT

public:
    explicit FunctionGenerator(QWidget* parent = nullptr);
    ~FunctionGenerator();

    enum Func : int
    {
        Sin,
        Cos,

        InSin,
        OutSin,
        InOutSin,

        InQuad,
        OutQuad,
        InOutQuad,

        InCubic,
        OutCubic,
        InOutCubic,

        InQuart,
        OutQuart,
        InOutQuart,

        InQuint,
        OutQuint,
        InOutQuint,

        InExpo,
        OutExpo,
        InOutExpo,

        InCirc,
        OutCirc,
        InOutCirc,

        InBack,
        OutBack,
        InOutBack,

        InElastic,
        OutElastic,
        InOutElastic,

        InBounce,
        OutBounce,
        InOutBounce
    };
    Q_ENUM(Func)

private:
    void Generate();

public:
    std::unique_ptr<Ui::FunctionGenerator> ui;
};

#endif // FUNCTIONGENERATOR_H
