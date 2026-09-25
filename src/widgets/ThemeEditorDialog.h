#pragma once
#include "../core/ThemeTokens.h"
#include <QDialog>
#include <QMap>

class QCheckBox;
class QComboBox;
class QFormLayout;
class QLabel;
class QLineEdit;
class QPushButton;
class QTabWidget;

// Stage 12: canlı tema düzenleyici — token renklerini renk seçicilerle
// düzenler (anlık önizleme), JSON olarak kaydeder; AI sekmesi Ollama ile
// tema üretir (doğrulamalı).
class ThemeEditorDialog : public QDialog {
    Q_OBJECT
public:
    explicit ThemeEditorDialog(const QString& baseTheme, QWidget* parent = nullptr);
    QString savedName() const { return m_saved; } // Kaydet'e basıldıysa tema adı

private slots:
    void pickColor(const QString& key);
    void onNameChanged();
    void onDarkToggled(bool on);
    void saveTheme();
    void refreshModels();
    void generateWithAi();

private:
    void buildColorRows(QWidget* parent);
    void addColorRow(QFormLayout* form, const QString& key, const QString& label);
    void syncButtons();
    void preview() const;
    void loadTokens(const ThemeTokens& tk);
    QColor tokenColor(const QString& key) const;
    void setTokenColor(const QString& key, const QColor& c);
    QString aiSystemPrompt() const;

    ThemeTokens m_tokens;
    QString m_saved;
    QLineEdit* m_name;
    QCheckBox* m_dark;
    QMap<QString, QPushButton*> m_buttons;
    // AI sekmesi
    QLineEdit* m_prompt;
    QComboBox* m_model;
    QLabel* m_aiStatus;
    QPushButton* m_generate;
    class OllamaClient* m_client = nullptr;
};
