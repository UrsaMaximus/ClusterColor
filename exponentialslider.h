#ifndef EXPONENTIALSCALESLIDER_H
#define EXPONENTIALSCALESLIDER_H

#include <QWidget>

namespace Ui
{
class ExponentialSlider;
}

class ExponentialSlider : public QWidget
{
    Q_OBJECT

public:
    explicit ExponentialSlider(QWidget *parent = nullptr);
    ~ExponentialSlider();

    float value();
    void setValue(float value);

    void setEndcapBehavior(bool lowerEndcap, float lowerEndcapValue, bool upperEndcap, float upperEndcapValue);
    void setExponentialMapping(float base, float exponentMin, float exponentMax);

signals:
    void onValueChanged(float value);

private slots:
    void on_horizontalSlider_valueChanged(int value);

    void on_signLabel_linkActivated(const QString &link);

private:
    Ui::ExponentialSlider *ui;

    void updateSignLabel();

    bool _enableLowerEndcap;
    bool _enableUpperEndcap;
    float _lowerEndcapValue;
    float _upperEndcapValue;
    float _base;
    float _exponentMin;
    float _exponentMax;
    bool _negative;
};

#endif // EXPONENTIALSCALESLIDER_H
