#include "automaticgroupgeneration.h"
#include "ui_automaticgroupgeneration.h"
#include "palette.h"
#include <QCloseEvent>
#include <QLayout>

AutomaticGroupGeneration::AutomaticGroupGeneration(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AutomaticGroupGeneration)
{
    ui->setupUi(this);

    numberOfClusters = 4;

    ui->colorGroupCount->setValue(numberOfClusters);
    ui->hueBoost->setValue(PaletteColor::HueWeight*100);
    ui->valueWeight->setValue(PaletteColor::ValueWeight*100);
    ui->saturationWeight->setValue(PaletteColor::SaturationWeight*100);
    ui->CIEDistance->setChecked(PaletteColor::UseCIEDistance);
}

void AutomaticGroupGeneration::on_colorGroupCount_valueChanged(int arg1)
{
    numberOfClusters = arg1;

    if (ui->autoGenerate->isChecked())
        generateGroups(numberOfClusters, true);
}

void AutomaticGroupGeneration::on_saturationWeight_valueChanged(int value)
{
    PaletteColor::SaturationWeight = (float)value / 100.0f;

    if (ui->autoGenerate->isChecked())
        generateGroups(numberOfClusters, true);
}

void AutomaticGroupGeneration::on_valueWeight_valueChanged(int value)
{
    PaletteColor::ValueWeight = (float)value / 100.0f;

    if (ui->autoGenerate->isChecked())
        generateGroups(numberOfClusters, true);
}

void AutomaticGroupGeneration::on_hueBoost_valueChanged(int value)
{
    PaletteColor::HueWeight = (float)value / 100.0f;

    if (ui->autoGenerate->isChecked())
        generateGroups(numberOfClusters, true);
}

void AutomaticGroupGeneration::on_CIEDistance_toggled(bool checked)
{
    PaletteColor::UseCIEDistance = checked;
    if (ui->autoGenerate->isChecked())
        generateGroups(numberOfClusters, true);
    ui->hueBoost->setEnabled(!checked);
}

void AutomaticGroupGeneration::on_regenerateIndex_clicked()
{
    generateGroups();
}

void AutomaticGroupGeneration::on_autoGenerate_toggled(bool checked)
{
    if (checked)
        generateGroups(numberOfClusters, true);
}

void AutomaticGroupGeneration::generateGroups()
{
    generateGroups(numberOfClusters, true);
    accept();
}

AutomaticGroupGeneration::~AutomaticGroupGeneration()
{
    delete ui;
}

void AutomaticGroupGeneration::on_cancelButton_clicked()
{
    reject();
}
