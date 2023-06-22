#define _USE_MATH_DEFINES
#include "functiongenerator.h"
#include "ui_functiongenerator.h"
#include <QMetaEnum>
#include <cmath>
#include <functional>

FunctionGenerator::FunctionGenerator(QWidget *parent) :
    QDialog(parent),
    ui(std::make_unique<Ui::FunctionGenerator>())
{
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    ui->setupUi(this);
    ui->graphPreview->setInteractive(false);
    ui->graphPreview->setGraphFlags(Graph::LIMIT_GRAPH_X);

    const auto e = QMetaEnum::fromType<Func>();
    for (int i = 0; i < e.keyCount(); ++i)
        ui->comboFunctions->addItem(e.key(i), e.value(i));
    Generate();

    connect(ui->numPoints, QOverload<int>::of(&QSpinBox::valueChanged), this, &FunctionGenerator::Generate);
    connect(ui->comboFunctions, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &FunctionGenerator::Generate);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &FunctionGenerator::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &FunctionGenerator::reject);
}

FunctionGenerator::~FunctionGenerator()
{
}

void FunctionGenerator::Generate()
{
    ui->graphPreview->setRange(-0.1, 1.1, -0.1, 1.1);
    std::function<double(double)> func = [](double t) { return t; };

    std::function<double(double)> outBounce = [](double x) {
        const double n1 = 7.5625;
        const double d1 = 2.75;
        if (x < 1 / d1) {
            return n1 * x * x;
        } else if (x < 2 / d1) {
            return n1 * std::pow(x - 1.5 / d1, 2) + 0.75;
        } else if (x < 2.5 / d1) {
            return n1 * std::pow(x - 2.25 / d1, 2) + 0.9375;
        } else {
            return n1 * std::pow(x - 2.625 / d1, 2) + 0.984375;
        }
    };
    std::function<double(double)> inBounce = [&](double x) { return 1.0 - outBounce(1.0 - x); };
    std::function<double(double)> inOutBounce = [&](double x) {
        return x < 0.5
                ? (1 - outBounce(1 - 2 * x)) / 2
                : (1 + outBounce(2 * x - 1)) / 2; };

    const auto e = static_cast<Func>(ui->comboFunctions->currentData().toInt());
    switch (e)
    {
    case Sin:
        ui->graphPreview->setRange(-0.1, 1.1, -1.1, 1.1);
        func = [](double x) { return std::sin(x * M_PI * 2.0); };
        break;
    case Cos:
        ui->graphPreview->setRange(-0.1, 1.1, -1.1, 1.1);
        func = [](double x) { return std::cos(x * M_PI * 2.0); };
        break;
    case InSin:
        func = [](double x) { return 1.0 - std::cos(x * M_PI * 0.5); };
        break;
    case OutSin:
        func = [](double x) { return std::sin(x * M_PI * 0.5); };
        break;
    case InOutSin:
        func = [](double x) { return -std::cos(M_PI * x)*0.5 + 0.5; };
        break;
    case InQuad:
        func = [](double x) { return x * x; };
        break;
    case OutQuad:
        func = [](double x) { return 1 - (1 - x) * (1 - x); };
        break;
    case InOutQuad:
        func = [](double x) { return x < 0.5 ? 2 * x * x : 1 - std::pow(-2 * x + 2, 2) / 2; };
        break;
    case InCubic:
        func = [](double x) { return x * x * x; };
        break;
    case OutCubic:
        func = [](double x) { return 1 - std::pow(1 - x, 3); };
        break;
    case InOutCubic:
        func = [](double x) { return x < 0.5 ? 4 * x * x * x : 1 - std::pow(-2 * x + 2, 3) / 2; };
        break;
    case InQuart:
        func = [](double x) { return x * x * x * x; };
        break;
    case OutQuart:
        func = [](double x) { return 1 - std::pow(1 - x, 4); };
        break;
    case InOutQuart:
        func = [](double x) { return x < 0.5 ? 8 * x * x * x * x : 1 - std::pow(-2 * x + 2, 4) / 2; };
        break;
    case InQuint:
        func = [](double x) { return x * x * x * x * x; };
        break;
    case OutQuint:
        func = [](double x) { return 1 - std::pow(1 - x, 5); };
        break;
    case InOutQuint:
        func = [](double x) { return x < 0.5 ? 16 * x * x * x * x * x : 1 - std::pow(-2 * x + 2, 5) / 2; };
        break;
    case InExpo:
        func = [](double x) { return x == 0 ? 0 : std::pow(2, 10 * x - 10); };
        break;
    case OutExpo:
        func = [](double x) { return x == 1 ? 1 : 1 - std::pow(2, -10 * x); };
        break;
    case InOutExpo:
        func = [](double x) { return x == 0
                    ? 0
                    : x == 1
                    ? 1
                    : x < 0.5 ? std::pow(2, 20 * x - 10) / 2
                    : (2 - std::pow(2, -20 * x + 10)) / 2; };
        break;
    case InCirc:
        func = [](double x) { return 1 - std::sqrt(1 - std::pow(x, 2)); };
       break;
    case OutCirc:
        func = [](double x) { return std::sqrt(1 - std::pow(x - 1, 2)); };
        break;
    case InOutCirc:
        func = [](double x) { return x < 0.5
                    ? (1 - std::sqrt(1 - std::pow(2 * x, 2))) / 2
                    : (std::sqrt(1 - std::pow(-2 * x + 2, 2)) + 1) / 2; };
        break;
    case InBack:
        func = [](double x) {
            const double c1 = 1.70158;
            const double c3 = c1 + 1;
            return c3 * x * x * x - c1 * x * x; };
        break;
    case OutBack:
        func = [](double x) {
            const double c1 = 1.70158;
            const double c3 = c1 + 1;
            return 1 + c3 * std::pow(x - 1, 3) + c1 * std::pow(x - 1, 2); };
        break;
    case InOutBack:
        func = [](double x) {
            const double c1 = 1.70158;
            const double c2 = c1 * 1.525;
            return x < 0.5
              ? (std::pow(2 * x, 2) * ((c2 + 1) * 2 * x - c2)) / 2
              : (std::pow(2 * x - 2, 2) * ((c2 + 1) * (x * 2 - 2) + c2) + 2) / 2;
        };
        break;
    case InElastic:
        func = [](double x) {
            const double c4 = (2 * M_PI) / 3;
            return x == 0
              ? 0
              : x == 1
              ? 1
              : -std::pow(2, 10 * x - 10) * std::sin((x * 10 - 10.75) * c4);
        };
        break;
    case OutElastic:
        func = [](double x) {
            const double c4 = (2 * M_PI) / 3;
            return x == 0
              ? 0
              : x == 1
              ? 1
              : std::pow(2, -10 * x) * std::sin((x * 10 - 0.75) * c4) + 1;
        };
        break;
    case InOutElastic:
        func = [](double x) {
            const double c5 = (2 * M_PI) / 4.5;
            return x == 0
              ? 0
              : x == 1
              ? 1
              : x < 0.5
              ? -(std::pow(2, 20 * x - 10) * std::sin((20 * x - 11.125) * c5)) / 2
              : (std::pow(2, -20 * x + 10) * std::sin((20 * x - 11.125) * c5)) / 2 + 1;
        };
        break;
    case InBounce:
        func = inBounce;
        break;
    case OutBounce:
        func = outBounce;
        break;
    case InOutBounce:
        func = inOutBounce;
        break;
    default:
        break;
    }
    std::vector<QPointF> points;
    points.reserve(ui->numPoints->value() + 1);
    const double increment = 1.0 / (ui->numPoints->value() - 1);
    for (int i = 0; i < ui->numPoints->value(); ++i)
        points.emplace_back(static_cast<double>(i) / (ui->numPoints->value() - 1), func(i * increment));
    ui->graphPreview->setPoints(points);
}
