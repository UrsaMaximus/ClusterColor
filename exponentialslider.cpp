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

    return pow(_base, exp) * (_negative ? -1.0f : 1.0f);
}

void ExponentialSlider::setValue(float newValue)
{
    _negative = newValue < 0;
    updateSignLabel();
    newValue = abs(newValue);

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

	 emit onValueChanged(value());
}

void ExponentialSlider::setExponentialMapping(float base, float exponentMin, float exponentMax)
{
    _base = base;
    _exponentMin = exponentMin;
    _exponentMax = exponentMax;

	emit onValueChanged(value());
}

void ExponentialSlider::on_horizontalSlider_valueChanged(int)
{
    ui->valueLabel->setText(QString("%1x").arg(QString::number(abs(value()), 'f', 2)));
	emit onValueChanged(value());
}

void ExponentialSlider::on_signLabel_linkActivated(const QString&)
{
    _negative = !_negative;
    updateSignLabel();
	emit onValueChanged(value());
}

void ExponentialSlider::updateSignLabel()
{
    ui->signLabel->setText(QString("<html><head/><body><a href=\"#\">%1</a></body></html>").arg(_negative ? "-" : "+"));
}
