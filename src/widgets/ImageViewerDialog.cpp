#include "ImageViewerDialog.h"
#include <QDialogButtonBox>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

ImageViewerDialog::ImageViewerDialog(const QString& path, QWidget* parent)
    : QDialog(parent) {
    setWindowTitle("Görüntü: " + QFileInfo(path).fileName());
    resize(640, 520);
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    auto* bar = new QHBoxLayout();
    auto* bIn = new QPushButton("+", this);
    auto* bOut = new QPushButton("−", this);
    auto* bFit = new QPushButton("Sığdır", this);
    bIn->setFixedWidth(36);
    bOut->setFixedWidth(36);
    bar->addWidget(bIn);
    bar->addWidget(bOut);
    bar->addWidget(bFit);
    bar->addStretch(1);
    lay->addLayout(bar);
    m_scroll = new QScrollArea(this);
    m_scroll->setWidgetResizable(false);
    m_scroll->setAlignment(Qt::AlignCenter);
    m_label = new QLabel(m_scroll);
    m_scroll->setWidget(m_label);
    lay->addWidget(m_scroll, 1);
    auto* box = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(box, &QDialogButtonBox::rejected, this, &ImageViewerDialog::reject);
    lay->addWidget(box);
    connect(bIn, &QPushButton::clicked, this, &ImageViewerDialog::zoomIn);
    connect(bOut, &QPushButton::clicked, this, &ImageViewerDialog::zoomOut);
    connect(bFit, &QPushButton::clicked, this, &ImageViewerDialog::zoomFit);
    m_pix = QPixmap(path);
    if (m_pix.isNull()) {
        m_label->setText("(görüntü açılamadı)");
        return;
    }
    zoomFit();
}

void ImageViewerDialog::applyZoom() {
    if (m_pix.isNull()) return;
    m_label->setPixmap(m_pix.scaled(
        int(m_pix.width() * m_zoom), int(m_pix.height() * m_zoom),
        Qt::KeepAspectRatio, Qt::SmoothTransformation));
    m_label->resize(m_label->pixmap().size());
    setWindowTitle(QString("Görüntü (%%1)").arg(int(m_zoom * 100)));
}

void ImageViewerDialog::zoomIn() {
    m_zoom = qMin(8.0, m_zoom * 1.25);
    applyZoom();
}

void ImageViewerDialog::zoomOut() {
    m_zoom = qMax(0.1, m_zoom / 1.25);
    applyZoom();
}

void ImageViewerDialog::zoomFit() {
    if (m_pix.isNull()) return;
    const QSize avail = m_scroll->viewport()->size();
    if (avail.isEmpty()) {
        m_zoom = 1.0;
    } else {
        m_zoom = qMin(1.0, qMin(double(avail.width()) / m_pix.width(),
                                double(avail.height()) / m_pix.height()));
        if (m_zoom <= 0) m_zoom = 1.0;
    }
    applyZoom();
}
