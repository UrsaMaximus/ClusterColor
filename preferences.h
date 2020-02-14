#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <QDialog>
#include <QSettings>

namespace Ui {
class Preferences;
}

class Preferences : public QDialog
{
    Q_OBJECT

public:
    explicit Preferences(QWidget *parent = nullptr);

    void WriteDefaults(); // Write defaults
    void Apply();  // Save to file
    void Revert(); // Load from file

    // Settings Getters
    bool PaletteExportAutosize();
    int PaletteExportHeight();
    int PaletteExportWidth();
    bool PaletteExportIncludeMetadata();

    bool GetSimpleBoolSetting(QString settingKey, bool defaultVal);
    void SetSimpleBoolSetting(QString settingKey, bool value);

    ~Preferences();

private slots:
    void on_buttonBox_accepted();

    void on_paletteFixedSize_toggled(bool checked);

private:
    Ui::Preferences *ui;
    QSettings* _settings;
    void setDefault(QString key, QVariant value);
};

#endif // PREFERENCES_H
