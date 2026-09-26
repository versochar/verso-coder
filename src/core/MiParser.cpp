#include "MiParser.h"

static void skipWs(const QString& s, int& p) {
    while (p < s.size() && (s[p] == ' ' || s[p] == '\t')) ++p;
}

QString MiParser::parseCString(const QString& s, int& pos) {
    QString out;
    if (pos >= s.size() || s[pos] != '"') return out;
    ++pos;
    while (pos < s.size()) {
        QChar ch = s[pos++];
        if (ch == '"') break;
        if (ch == '\\' && pos < s.size()) {
            QChar e = s[pos++];
            if (e == 'n') out += '\n';
            else if (e == 't') out += '\t';
            else if (e == 'r') out += '\r';
            else out += e; // \" \\ \/ ...
        } else out += ch;
    }
    return out;
}

QVariant MiParser::parseValue(const QString& s, int& pos) {
    skipWs(s, pos);
    if (pos >= s.size()) return QVariant();
    QChar ch = s[pos];
    if (ch == '"') return parseCString(s, pos);
    if (ch == '{') {
        ++pos;
        QVariantMap m;
        while (pos < s.size() && s[pos] != '}') {
            const int mark = pos; // Stage 31: ilerleme garantisi (fuzz bulgusu)
            skipWs(s, pos);
            QString key;
            while (pos < s.size() && (s[pos].isLetterOrNumber() || s[pos] == '_' || s[pos] == '-'))
                key += s[pos++];
            if (pos < s.size() && s[pos] == '=') ++pos;
            m[key] = parseValue(s, pos);
            skipWs(s, pos);
            if (pos < s.size() && s[pos] == ',') ++pos;
            if (pos == mark) ++pos; // çöp karakter: zorla ilerle
        }
        if (pos < s.size() && s[pos] == '}') ++pos;
        return m;
    }
    if (ch == '[') {
        ++pos;
        QVariantList l;
        while (pos < s.size() && s[pos] != ']') {
            skipWs(s, pos);
            if (pos < s.size() && s[pos] == ']') break;
            // Liste öğesi: ya {..}/{..} (sonuçsuz) ya da k=v
            if (pos < s.size() && (s[pos] == '{' || s[pos] == '"' || s[pos] == '[')) {
                l << parseValue(s, pos);
            } else {
                // k=v biçimi → tek anahtarlı map
                QString key;
                while (pos < s.size() && (s[pos].isLetterOrNumber() || s[pos] == '_' || s[pos] == '-'))
                    key += s[pos++];
                if (pos < s.size() && s[pos] == '=') {
                    ++pos;
                    QVariantMap m;
                    m[key] = parseValue(s, pos);
                    l << m;
                } else if (!key.isEmpty()) {
                    l << key;
                    // "..." olmayan yalın değerleri atla
                    while (pos < s.size() && s[pos] != ',' && s[pos] != ']') ++pos;
                } else {
                    while (pos < s.size() && s[pos] != ',' && s[pos] != ']') ++pos;
                }
            }
            skipWs(s, pos);
            if (pos < s.size() && s[pos] == ',') ++pos;
        }
        if (pos < s.size() && s[pos] == ']') ++pos;
        return l;
    }
    // Yalın atom (sayı vb.) → metin olarak al
    QString atom;
    while (pos < s.size() && s[pos] != ',' && s[pos] != '}' && s[pos] != ']')
        atom += s[pos++];
    return atom.trimmed();
}

MiRecord MiParser::parseLine(const QString& line) {
    MiRecord r;
    QString s = line.trimmed();
    if (s.isEmpty()) { r.kind = "empty"; return r; }
    QChar c0 = s[0];
    if (c0 == '~' || c0 == '@' || c0 == '&') {
        r.kind = (c0 == '~') ? "console" : (c0 == '@' ? "target" : "log");
        int p = 1;
        r.stream = parseCString(s, p);
        return r;
    }
    if (s == "(gdb)") { r.kind = "prompt"; return r; }
    int p = 0;
    // token: baştaki rakamlar
    while (p < s.size() && s[p].isDigit()) r.token += s[p++];
    if (p >= s.size()) { r.kind = "unknown"; return r; }
    QChar pre = s[p++];
    if (pre == '^') r.kind = "result";
    else if (pre == '*') r.kind = "exec";
    else if (pre == '+') r.kind = "status";
    else if (pre == '=') r.kind = "notify";
    else { r.kind = "unknown"; return r; }
    // sınıf: virgüle kadar
    while (p < s.size() && s[p] != ',') r.cls += s[p++];
    if (p < s.size() && s[p] == ',') ++p;
    // sonuç listesi: k=v,k=v
    while (p < s.size()) {
        skipWs(s, p);
        QString key;
        while (p < s.size() && (s[p].isLetterOrNumber() || s[p] == '_' || s[p] == '-'))
            key += s[p++];
        if (p < s.size() && s[p] == '=') ++p;
        else break;
        r.fields[key] = parseValue(s, p);
        skipWs(s, p);
        if (p < s.size() && s[p] == ',') ++p;
    }
    return r;
}

QString MiParser::frameFile(const QVariantMap& frame) {
    QString f = frame.value("fullname").toString();
    if (!f.isEmpty()) return f;
    return frame.value("file").toString();
}

int MiParser::frameLine(const QVariantMap& frame) {
    return frame.value("line").toInt() - 1; // 0-based (-1 = bilinmiyor)
}
