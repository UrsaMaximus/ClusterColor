#include "linearslider.h"
#include "ui_linearslider.h"
#include <math.h>

LinearSlider::LinearSlider(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LinearSlider)
{
    ui->setupUi(this);
    _value = 0;
    _rangeIndex = 0;
    _resoulutonRatio = 10;

    setRangeHalfWidth(100);
    setValueUnit("%");
    setShowRangeButtons(false);
}

LinearSlider::~LinearSlider()
{
    delete ui;
}

int LinearSlider::rangeHalfWidth()
{
    return _rangeHalfWidth;
}

void LinearSlider::setRangeHalfWidth(int value)
{
    _rangeHalfWidth = value;
    updateRange();
}

float LinearSlider::value()
{
    return _value;
}

bool LinearSlider::showRangeButtons()
{
    return ui->rangeUpButton->isVisible();
}

void LinearSlider::setShowRangeButtons(bool show)
{
    ui->rangeUpButton->setVisible(show);
    ui->rangeDownButton->setVisible(show);
}

QString LinearSlider::valueUnit()
{
    return _valueUnit;
}
void LinearSlider::setValueUnit(QString unit)
{
    _valueUnit = unit;

    updateText();
}

void LinearSlider::updateRange()
{
    ui->horizontalSlider->setRange((_rangeIndex-1)*_resoulutonRatio*_rangeHalfWidth, (_rangeIndex+1)*_resoulutonRatio*_rangeHalfWidth);
}

void LinearSlider::updateText()
{
    ui->valueLabel->setText(QString("%1%2").arg(ui->horizontalSlider->value()/_resoulutonRatio).arg(_valueUnit));
    ui->lowerRangeLabel->setText(QString("%1%2").arg(ui->horizontalSlider->minimum()/_resoulutonRatio).arg(_valueUnit));
    ui->upperRangeLabel->setText(QString("%1%2").arg(ui->horizontalSlider->maximum()/_resoulutonRatio).arg(_valueUnit));
}

void LinearSlider::setValue(float value)
{
    // Check if the slider needs to change range to accomodate the current value
    if (round(value * _resoulutonRatio) < ui->horizontalSlider->minimum() || round(value * _resoulutonRatio) > ui->horizontalSlider->maximum())
    {
        _rangeIndex = ((int)round(value * _resoulutonRatio))/_resoulutonRatio*_rangeHalfWidth;
        updateRange();
    }
    ui->horizontalSlider->setValue((int)round(value * _resoulutonRatio));
}

void LinearSlider::on_rangeUpButton_clicked()
{
    _rangeIndex++;
    updateRange();
}

void LinearSlider::on_rangeDownButton_clicked()
{
    _rangeIndex--;
    updateRange();
}

void LinearSlider::on_horizontalSlider_valueChanged(int value)
{
    ui->valueLabel->setText(QString("%1%2").arg(value/_resoulutonRatio).arg(_valueUnit));
    _value = (float)value/(float)_resoulutonRatio;
    onValueChanged(_value);
}

void LinearSlider::on_horizontalSlider_rangeChanged(int min, int max)
{
    ui->lowerRangeLabel->setText(QString("%1%2").arg(min/_resoulutonRatio).arg(_valueUnit));
    ui->upperRangeLabel->setText(QString("%1%2").arg(max/_resoulutonRatio).arg(_valueUnit));
}
