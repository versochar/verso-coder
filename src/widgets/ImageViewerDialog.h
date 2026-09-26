#pragma once
#include <QDialog>

class QLabel;
class QScrollArea;

// Stage 28: görüntü görüntüleyici (PNG/SVG/JPG/GIF + yakınlaştırma).
class ImageViewerDialog : public QDialog {
    Q_OBJECT
public:
    explicit ImageViewerDialog(const QString& path, QWidget* parent = nullptr);

private slots:
    void zoomIn();
    void zoomOut();
    void zoomFit();

private:
    void applyZoom();
    QLabel* m_label = nullptr;
    QScrollArea* m_scroll = nullptr;
    QPixmap m_pix;
    double m_zoom = 1.0;
};
