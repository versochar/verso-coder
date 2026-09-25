#pragma once
#include <QObject>
#include <QString>

class QLabel;
class QLayoutItem;
class QTimer;
class QVBoxLayout;
class QWidget;

// Stage 10: toast bildirimleri — sağ altta yığılan, otomatik kaybolan,
// animasyonlu (azaltılmış hareket destekli) kısa mesajlar.
enum class ToastType { Info, Success, Warning, Error };

class ToastManager : public QObject {
    Q_OBJECT
public:
    static ToastManager& instance();

    // Toast'ların çizileceği ana pencereyi kaydet (-overlay onun üzerine açılır)
    void attach(QWidget* hostWindow);
    bool isAttached() const { return m_host != nullptr; }

    void show(ToastType type, const QString& message, int ms = 2600);
    int visibleCount() const;

    static QString typeString(ToastType t); // "info" | "success" | ...

    // Test/erişilebilirlik: tüm toast'ları hemen kapat
    void dismissAll();

private slots:
    void onHostResized();

private:
    explicit ToastManager(QObject* parent = nullptr);
    bool eventFilter(QObject* o, QEvent* e) override;
    QWidget* m_host = nullptr;
    QWidget* m_overlay = nullptr;
    QVBoxLayout* m_lay = nullptr;
    QTimer* m_relayout = nullptr;
};
