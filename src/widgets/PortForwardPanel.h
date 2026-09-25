#pragma once
#include "../core/PortForwarder.h"
#include <QTableWidget>
#include <QWidget>

// Stage 16: port yönlendirme paneli — aktif tüneller + yeni kural.
class PortForwardPanel : public QWidget {
    Q_OBJECT
public:
    explicit PortForwardPanel(QWidget* parent = nullptr);

    void setForwarder(PortForwarder* f);

signals:
    void statusMessage(const QString& msg);

private slots:
    void refresh();
    void addRule();
    void removeSelected();

private:
    QTableWidget* m_table;
    PortForwarder* m_fwd = nullptr;
};
