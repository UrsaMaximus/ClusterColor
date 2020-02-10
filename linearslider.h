#ifndef PERCENTAGESHIFTSLIDER_H
#define PERCENTAGESHIFTSLIDER_H

#include <QWidget>

namespace Ui {
class LinearSlider;
}

class LinearSlider : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(QString unit READ valueUnit WRITE setValueUnit)
    Q_PROPERTY(int rangeHalfWidth READ rangeHalfWidth WRITE setRangeHalfWidth)
    Q_PROPERTY(bool showRangeButtons READ showRangeButtons WRITE setShowRangeButtons)
    Q_PROPERTY(float value READ value WRITE setValue NOTIFY onValueChanged)

public:
    explicit LinearSlider(QWidget *parent = nullptr);
    ~LinearSlider();

    float value();
    void setValue(float value);

    int rangeHalfWidth();
    void setRangeHalfWidth(int value);

    QString valueUnit();
    void setValueUnit(QString unit);

    bool showRangeButtons();
    void setShowRangeButtons(bool show);

signals:
    void onValueChanged(float value);

private slots:
    void on_rangeUpButton_clicked();

    void on_rangeDownButton_clicked();

    void on_horizontalSlider_valueChanged(int value);

    void on_horizontalSlider_rangeChanged(int min, int max);

private:
    Ui::LinearSlider *ui;

    void updateRange();
    void updateText();

    float _value;
    int _rangeIndex;
    int _resoulutonRatio;
    int _rangeHalfWidth;

    QString _valueUnit;
};

#endif // PERCENTAGESHIFTSLIDER_H
