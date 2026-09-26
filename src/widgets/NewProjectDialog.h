#pragma once
#include <QDialog>
#include <QMap>
#include <QString>

// Stage 30: yeni proje sihirbazı — şablon + ad + konum → iskelet dosyalar.
class NewProjectDialog : public QDialog {
    Q_OBJECT
public:
    explicit NewProjectDialog(QWidget* parent = nullptr);
    // Oluşturulan kök (boşsa vazgeçildi)
    QString createdRoot() const { return m_created; }

private slots:
    void create();

private:
    // Şablon adı → dosya listesi (göreli yol → içerik üreteci anahtarı)
    static QMap<QString, QString> templateFiles(const QString& tpl,
                                                const QString& name);
    class QComboBox* m_tpl = nullptr;
    class QLineEdit* m_name = nullptr;
    class QLineEdit* m_dir = nullptr;
    QString m_created;
};
