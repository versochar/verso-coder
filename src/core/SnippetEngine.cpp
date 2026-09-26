#include "SnippetEngine.h"
#include <QDate>
#include <QMap>
#include <QProcess>
#include <QRegularExpression>
#include <QSet>
#include <algorithm>

QList<SnippetSegment> SnippetEngine::parse(const QString& body) {
    QList<SnippetSegment> out;
    QString cur;
    auto flush = [&]() {
        if (!cur.isEmpty()) {
            SnippetSegment s;
            s.type = SnippetSegment::Type::Text;
            s.text = cur;
            out << s;
            cur.clear();
        }
    };
    QSet<int> seen; // ilk görülen durak = Tabstop/Placeholder, sonrası Mirror
    int i = 0;
    const int n = body.size();
    while (i < n) {
        const QChar c = body[i];
        if (c == '$' && i + 1 < n) {
            const QChar d = body[i + 1];
            if (d.isDigit()) {
                int j = i + 1;
                while (j < n && body[j].isDigit()) ++j;
                const int idx = body.mid(i + 1, j - i - 1).toInt();
                flush();
                SnippetSegment s;
                s.index = idx;
                s.type = seen.contains(idx) ? SnippetSegment::Type::Mirror
                                            : SnippetSegment::Type::Tabstop;
                seen.insert(idx);
                out << s;
                i = j;
                continue;
            }
            if (d == '{') {
                // ${1:varsayılan} | ${1} | ${TM_...} handled by expandVars önce
                int j = i + 2;
                QString num;
                while (j < n && body[j].isDigit()) num += body[j++];
                if (!num.isEmpty() && j < n && body[j] == ':') {
                    ++j;
                    int depth = 1;
                    QString def;
                    while (j < n && depth > 0) {
                        if (body[j] == '{') ++depth;
                        else if (body[j] == '}') {
                            --depth;
                            if (depth == 0) { ++j; break; }
                        }
                        if (depth > 0) def += body[j];
                        ++j;
                    }
                    flush();
                    const int idx = num.toInt();
                    SnippetSegment s;
                    s.index = idx;
                    if (seen.contains(idx)) {
                        s.type = SnippetSegment::Type::Mirror;
                    } else {
                        s.type = SnippetSegment::Type::Placeholder;
                        s.text = def;
                    }
                    seen.insert(idx);
                    out << s;
                    i = j;
                    continue;
                }
                if (!num.isEmpty() && j < n && body[j] == '}') {
                    flush();
                    const int idx = num.toInt();
                    SnippetSegment s;
                    s.index = idx;
                    s.type = seen.contains(idx) ? SnippetSegment::Type::Mirror
                                                : SnippetSegment::Type::Tabstop;
                    seen.insert(idx);
                    out << s;
                    i = j + 1;
                    continue;
                }
                // bilinmeyen ${...}: düz metin
                cur += c;
                ++i;
                continue;
            }
            if (d == '$') { cur += '$'; i += 2; continue; } // $$ kaçışı
        }
        cur += c;
        ++i;
    }
    flush();
    return out;
}

QString SnippetEngine::expandVars(QString body, const QString& fileName,
                                  const QString& selected) {
    // $TM_FILENAME / ${TM_FILENAME}, $TM_SELECTED_TEXT, $CLIPBOARD
    body.replace("${TM_FILENAME}", fileName);
    body.replace("$TM_FILENAME", fileName);
    body.replace("${TM_SELECTED_TEXT}", selected);
    body.replace("$TM_SELECTED_TEXT", selected);
    // Stage 25: tarih + yazar değişkenleri
    const QString today = QDate::currentDate().toString(Qt::ISODate);
    const QString year = QString::number(QDate::currentDate().year());
    QString author = qEnvironmentVariable("VERSO_AUTHOR");
    if (author.isEmpty()) {
        QProcess git;
        git.start("git", {"config", "user.name"});
        if (git.waitForFinished(2000)) author = QString::fromUtf8(git.readAllStandardOutput()).trimmed();
        if (author.isEmpty()) author = qEnvironmentVariable("USER");
    }
    for (const char* k : {"CURRENT_DATE", "TM_DATE"}) {
        body.replace(QString("${%1}").arg(k), today);
        body.replace(QString("$%1").arg(k), today);
    }
    for (const char* k : {"CURRENT_YEAR", "TM_YEAR"}) {
        body.replace(QString("${%1}").arg(k), year);
        body.replace(QString("$%1").arg(k), year);
    }
    body.replace("${TM_AUTHOR}", author);
    body.replace("$TM_AUTHOR", author);
    // $CLIPBOARD expand() içinde işlenir (clipboard parametresi)
    return body;
}

SnippetExpand SnippetEngine::expand(const QString& body, int baseOffset,
                                    const QString& clipboard) {
    SnippetExpand r;
    QString work = body;
    work.replace("$CLIPBOARD", clipboard);
    work.replace("${CLIPBOARD}", clipboard);
    QMap<int, int> firstOffset; // durak → ilk konum (ayna için)
    QMap<int, QString> defaults;
    int pos = baseOffset;
    for (const SnippetSegment& s : parse(work)) {
        switch (s.type) {
        case SnippetSegment::Type::Text:
            r.text += s.text;
            pos += s.text.size();
            break;
        case SnippetSegment::Type::Tabstop:
            if (!firstOffset.contains(s.index)) {
                firstOffset[s.index] = pos;
                r.stopOffsets << pos;
                r.stopNames << QString();
            }
            break;
        case SnippetSegment::Type::Placeholder:
            if (!firstOffset.contains(s.index)) {
                firstOffset[s.index] = pos;
                defaults[s.index] = s.text;
                r.stopOffsets << pos;
                r.stopNames << s.text;
                r.text += s.text;
                pos += s.text.size();
            } else {
                // daha önce görüldü: ayna gibi davran (varsayılanı tekrarla)
                r.text += defaults.value(s.index);
                pos += defaults.value(s.index).size();
            }
            break;
        case SnippetSegment::Type::Mirror:
            // Ayna: ilk yazılanı tekrarla (yer tutucu varsayılanı ya da boş)
            r.text += defaults.value(s.index);
            pos += defaults.value(s.index).size();
            break;
        }
    }
    // Durakları numara sırasına koy ($0 en sonda)
    QList<int> order = firstOffset.keys();
    std::sort(order.begin(), order.end(), [](int a, int b) {
        if (a == 0) return false;
        if (b == 0) return true;
        return a < b;
    });
    QList<int> sortedOff;
    QList<QString> sortedNames;
    for (int idx : order) {
        // stopOffsets stopNames ile paralel — konuma göre eşle
        const int off = firstOffset[idx];
        int k = r.stopOffsets.indexOf(off);
        sortedOff << off;
        sortedNames << (k >= 0 ? r.stopNames[k] : defaults.value(idx));
    }
    r.stopOffsets = sortedOff;
    r.stopNames = sortedNames;
    return r;
}

bool SnippetEngine::hasStops(const QString& body) {
    return body.contains('$');
}

int SnippetEngine::stopCount(const QString& body) {
    QSet<int> ids;
    for (const SnippetSegment& s : parse(body))
        if (s.type == SnippetSegment::Type::Tabstop
            || s.type == SnippetSegment::Type::Placeholder)
            ids.insert(s.index);
    return ids.size();
}
