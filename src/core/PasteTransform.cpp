#include "PasteTransform.h"
#include <algorithm>

QList<PasteTransform::Kind> PasteTransform::all() {
    return {Kind::Plain, Kind::JoinLines, Kind::JoinComma, Kind::Unique, Kind::Sort,
            Kind::SortUnique, Kind::Upper, Kind::Lower, Kind::Quote, Kind::Trim};
}

QString PasteTransform::title(Kind k) {
    switch (k) {
    case Kind::Plain: return "Düz yapıştır";
    case Kind::JoinLines: return "Satırları birleştir";
    case Kind::JoinComma: return "\"a\", \"b\" biçimi";
    case Kind::Unique: return "Yinelenenleri at";
    case Kind::Sort: return "Satırları sırala";
    case Kind::SortUnique: return "Sırala + tekilleştir";
    case Kind::Upper: return "BÜYÜK HARF";
    case Kind::Lower: return "küçük harf";
    case Kind::Quote: return "Her satırı tırnakla";
    case Kind::Trim: return "Boşlukları kırp";
    }
    return "Düz";
}

QString PasteTransform::apply(Kind k, const QString& text) {
    if (k == Kind::Plain) return text;
    QStringList lines = text.split('\n');
    // Sondaki boş satırı (son \n) at
    if (!lines.isEmpty() && lines.last().isEmpty()) lines.removeLast();
    switch (k) {
    case Kind::Plain: return text;
    case Kind::JoinLines: {
        QStringList t;
        for (const QString& l : lines) t << l.trimmed();
        return t.join(' ');
    }
    case Kind::JoinComma: {
        QStringList t;
        for (const QString& l : lines) t << "\"" + l.trimmed() + "\"";
        return t.join(", ");
    }
    case Kind::Unique: {
        QStringList seen, out;
        for (const QString& l : lines)
            if (!seen.contains(l)) { seen << l; out << l; }
        return out.join('\n');
    }
    case Kind::Sort: {
        QStringList t = lines;
        std::sort(t.begin(), t.end());
        return t.join('\n');
    }
    case Kind::SortUnique: {
        QStringList t = lines;
        std::sort(t.begin(), t.end());
        t.erase(std::unique(t.begin(), t.end()), t.end());
        return t.join('\n');
    }
    case Kind::Upper: return text.toUpper();
    case Kind::Lower: return text.toLower();
    case Kind::Quote: {
        QStringList t;
        for (const QString& l : lines) t << "\"" + l + "\"";
        return t.join('\n');
    }
    case Kind::Trim: {
        QStringList t;
        for (const QString& l : lines) t << l.trimmed();
        return t.join('\n');
    }
    }
    return text;
}
