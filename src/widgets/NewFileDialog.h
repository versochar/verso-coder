#pragma once
#include <QDialog>

class QLineEdit;
class QListWidget;

// Stage 20: yeni dosya dil seçici — 20 dil + Düz metin, süzme kutusu,
// en üstte son kullanılanlar (en çok 3).
class NewFileDialog : public QDialog {
    Q_OBJECT
public:
    // changeMode: başlık "Dosya Dilini Değiştir" olur, iskelet ipucu gizlenir
    explicit NewFileDialog(bool changeMode, QWidget* parent = nullptr);

    // Seçilen dil id ("python" | "plain" …); exec() reddedildiyse boş
    QString selectedLang() const { return m_lang; }

private slots:
    void onFilter(const QString& text);
    void onAccept();

private:
    void refill(const QString& filter);

    QLineEdit* m_filter = nullptr;
    QListWidget* m_list = nullptr;
    QString m_lang;
    bool m_changeMode = false;
};
