#pragma once
#include <QString>
#include <QWidget>

class QComboBox;
class QHBoxLayout;

// Dosya yolu + sembol (sınıf/fonksiyon) gezgini.
// Stage 11: dosya ikonu, tür önekli semboller (▣ sınıf, ƒ fonksiyon),
// grup başlıklı sembol menüsü.
struct CrumbSymbol {
    int line = 0;       // 1-based
    QString name;       // "Foo" / "bar()"
    QString kind;       // "class" | "func"
};

class BreadcrumbBar : public QWidget {
    Q_OBJECT
public:
    explicit BreadcrumbBar(QWidget* parent = nullptr);
    void setPath(const QString& root, const QString& filePath, const QString& docText);

    // Saf mantık (test için): metinden tür bilgili semboller
    static QList<CrumbSymbol> parseSymbols(const QString& text);

signals:
    void crumbClicked(const QString& path); // klasör/dosya tıklaması
    void symbolActivated(int line);

private:
    QHBoxLayout* m_lay;
    QComboBox* m_symbols;
};
