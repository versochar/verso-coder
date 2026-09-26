#pragma once
#include <QDialog>

class QComboBox;
class QSpinBox;

// Stage 21: ilk-çalıştırma sihirbazı — dil + tema + kısayol profili + font.
// Kabulde ayarları yazar, m_firstRun=false döner (MainWindow kaydeder).
class FirstRunDialog : public QDialog {
    Q_OBJECT
public:
    explicit FirstRunDialog(QWidget* parent = nullptr);

private slots:
    void onAccept();

private:
    QComboBox* m_lang = nullptr;
    QComboBox* m_theme = nullptr;
    QComboBox* m_keys = nullptr;
    QSpinBox* m_font = nullptr;
};
