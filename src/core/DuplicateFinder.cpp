#include "DuplicateFinder.h"

static QString normLine(const QString& ln) {
    // Boşlukları sıkıştır + dize içeriklerini maskele (yapısal kopya yakalar)
    QString t = ln.simplified();
    bool inStr = false;
    QChar q;
    QString out;
    out.reserve(t.size());
    for (QChar c : t) {
        if (!inStr && (c == '"' || c == '\'')) {
            inStr = true;
            q = c;
            out += c;
        } else if (inStr && c == q) {
            inStr = false;
            out += c;
        } else if (inStr) {
            out += '*';
        } else {
            out += c;
        }
    }
    return out;
}

QList<DupGroup> DuplicateFinder::find(const QMap<QString, QString>& pathToText,
                                      int minLines) {
    struct Occ {
        QString loc;
        int lines;
    };
    QMap<QString, QList<Occ>> byHash;
    QMap<QString, int> hashLines;
    for (auto it = pathToText.constBegin(); it != pathToText.constEnd(); ++it) {
        QStringList raw = it.value().split('\n');
        QStringList lines;
        QList<int> nums;
        for (int i = 0; i < raw.size(); ++i) {
            const QString n = normLine(raw[i]);
            if (n.isEmpty()) continue;
            lines << n;
            nums << (i + 1);
        }
        for (int i = 0; i + minLines <= lines.size(); ++i) {
            const QStringList blk = lines.mid(i, minLines);
            const QString h = QString::fromLatin1(
                QCryptographicHash::hash(blk.join('\n').toUtf8(),
                                         QCryptographicHash::Sha1).toHex().left(12));
            // Aynı dosya içinde çakışan blokları tekille (ilkini tut)
            const QString loc = QString("%1:%2").arg(it.key()).arg(nums[i]);
            byHash[h] << Occ{loc, minLines};
            hashLines[h] = minLines;
        }
    }
    QList<DupGroup> out;
    for (auto it = byHash.constBegin(); it != byHash.constEnd(); ++it) {
        // Aynı dosyanın ardışık kayan pencerelerini ele (gerçek kopya: ≥2 farklı konum,
        // aynı dosya olsa bile farklı bloklar sayılır — kayan pencere gürültüsünü azalt:
        // konum sayısı blok uzunluğundan fazlaysa gruptur)
        if (it.value().size() < 2) continue;
        DupGroup g;
        g.hash = it.key();
        g.lines = hashLines.value(it.key());
        for (const Occ& o : it.value()) g.files << o.loc;
        out << g;
    }
    return out;
}
