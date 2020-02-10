#ifndef AUTOMATICGROUPGENERATION_H
#define AUTOMATICGROUPGENERATION_H

#include <QDialog>

namespace Ui {
class AutomaticGroupGeneration;
}

class AutomaticGroupGeneration : public QDialog
{
    Q_OBJECT

public:
    explicit AutomaticGroupGeneration(QWidget *parent = nullptr);
    ~AutomaticGroupGeneration();

signals:
    void generateGroups(int groupCount, bool saveRecolors);
//    void accept();
//    void reject();

private slots:
    void on_colorGroupCount_valueChanged(int arg1);
    void on_saturationWeight_valueChanged(int value);
    void on_valueWeight_valueChanged(int value);
    void on_hueBoost_valueChanged(int value);
    void on_CIEDistance_toggled(bool checked);
    void on_regenerateIndex_clicked();
    void on_autoGenerate_toggled(bool checked);

    void on_cancelButton_clicked();

private:
    Ui::AutomaticGroupGeneration *ui;
    int numberOfClusters;
    void generateGroups();
};

#endif // AUTOMATICGROUPGENERATION_H
