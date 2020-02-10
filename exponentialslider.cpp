#include "exponentialslider.h"
#include "ui_exponentialslider.h"
#include <math.h>
#include <algorithm>

ExponentialSlider::ExponentialSlider(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ExponentialSlider)
{
    ui->setupUi(this);

    _enableLowerEndcap = true;
    _enableUpperEndcap = false;
    _lowerEndcapValue = 0;
    _upperEndcapValue = 256;
    _base = 4;
    _exponentMin = -1.25;
    _exponentMax = 0.75;
}

ExponentialSlider::~ExponentialSlider()
{
    delete ui;
}

float ExponentialSlider::value()
{
    int sliderVal = ui->horizontalSlider->value();

    if (_enableLowerEndcap && sliderVal == ui->horizontalSlider->minimum())
        return _lowerEndcapValue;

    if (_enableUpperEndcap && sliderVal == ui->horizontalSlider->maximum())
        return _upperEndcapValue;

    int sliderMin = _enableLowerEndcap ? ui->horizontalSlider->minimum()+1 : ui->horizontalSlider->minimum();
    int sliderMax = _enableUpperEndcap ? ui->horizontalSlider->maximum()-1 : ui->horizontalSlider->maximum();

    float exp = ((float)(sliderVal - sliderMin) / (float)(sliderMax-sliderMin) * (_exponentMax-_exponentMin)) + _exponentMin;

    return pow(_base, exp);
}

void ExponentialSlider::setValue(float newValue)
{
    float exp = log(newValue) / log(_base);

    if (exp < _exponentMin)
    {
        ui->horizontalSlider->setValue(ui->horizontalSlider->minimum());
        return;
    }
    if (exp > _exponentMax)
    {
        ui->horizontalSlider->setValue(ui->horizontalSlider->maximum());
        return;
    }

    int sliderMin = _enableLowerEndcap ? ui->horizontalSlider->minimum()+1 : ui->horizontalSlider->minimum();
    int sliderMax = _enableUpperEndcap ? ui->horizontalSlider->maximum()-1 : ui->horizontalSlider->maximum();

    int sliderVal = (int)round((exp - _exponentMin) / (_exponentMax-_exponentMin) * (float)(sliderMax-sliderMin) ) + sliderMin;

    int boundedSliderValue = std::clamp(sliderVal, sliderMin, sliderMax);

    ui->horizontalSlider->setValue(boundedSliderValue);
}

void ExponentialSlider::setEndcapBehavior(bool lowerEndcap, float lowerEndcapValue, bool upperEndcap, float upperEndcapValue)
{
    _enableLowerEndcap = lowerEndcap;
    _enableUpperEndcap = upperEndcap;
    _lowerEndcapValue = lowerEndcapValue;
    _upperEndcapValue = upperEndcapValue;

     onValueChanged(value());
}

void ExponentialSlider::setExponentialMapping(float base, float exponentMin, float exponentMax)
{
    _base = base;
    _exponentMin = exponentMin;
    _exponentMax = exponentMax;

    onValueChanged(value());
}

void ExponentialSlider::on_horizontalSlider_valueChanged(int slider)
{
    ui->valueLabel->setText(QString("%1x").arg(QString::number(value(), 'f', 2)));
    onValueChanged(value());
}
