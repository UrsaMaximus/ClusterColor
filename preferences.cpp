#include "preferences.h"
#include "ui_preferences.h"
#include <QSettings>
#include <QLayout>

Preferences::Preferences(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Preferences)
{
    ui->setupUi(this);
    _settings = new QSettings("GreatestBearStudios", "ClusterColor", this);
    _settings->sync();
    WriteDefaults();
    Revert();
}

Preferences::~Preferences()
{
    _settings->sync();
    delete ui;
}

void Preferences::setDefault(QString key, QVariant value)
{
    if (!_settings->contains(key))
        _settings->setValue(key, value);
}

void Preferences::WriteDefaults()
{
    setDefault("PaletteExportAutosize", true);
    setDefault("PaletteExportHeight", 16);
    setDefault("PaletteExportWidth", 16);
    setDefault("PaletteExportIncludeMetadata", true);
}

bool Preferences::PaletteExportAutosize()
{
    return _settings->value("PaletteExportAutosize").toBool();
}

int Preferences::PaletteExportHeight()
{
    return _settings->value("PaletteExportHeight").toInt();
}

int Preferences::PaletteExportWidth()
{
    return _settings->value("PaletteExportWidth").toInt();
}

bool Preferences::PaletteExportIncludeMetadata()
{
    return _settings->value("PaletteExportIncludeMetadata").toBool();
}

void Preferences::Revert()
{
    ui->paletteAutoSize->setChecked(PaletteExportAutosize());
    ui->paletteHeight->setValue(PaletteExportHeight());
    ui->paletteWidth->setValue(PaletteExportWidth());
    ui->includeMetadata->setChecked(PaletteExportIncludeMetadata());
}

void Preferences::Apply()
{
    _settings->setValue("PaletteExportAutosize", ui->paletteAutoSize->isChecked());
    _settings->setValue("PaletteExportHeight", ui->paletteHeight->value());
    _settings->setValue("PaletteExportWidth", ui->paletteWidth->value());
    _settings->setValue("PaletteExportIncludeMetadata", ui->includeMetadata->isChecked());
}

void Preferences::on_buttonBox_accepted()
{
    Apply();
}

void Preferences::on_paletteFixedSize_toggled(bool checked)
{
    ui->paletteWidth->setEnabled(checked);
    ui->paletteHeight->setEnabled(checked);
    ui->widthLabel->setEnabled(checked);
    ui->heightLabel->setEnabled(checked);
}
