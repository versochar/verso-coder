#include "PromptVars.h"
#include <QFileInfo>

QString PromptVars::expand(const QString& tpl, const QMap<QString, QString>& vars) {
    QString out = tpl;
    QMap<QString, QString> v = vars;
    if (!v.contains("date")) v["date"] = QDate::currentDate().toString(Qt::ISODate);
    for (auto it = v.constBegin(); it != v.constEnd(); ++it)
        out.replace("{{" + it.key() + "}}", it.value());
    return out;
}

QString PromptVars::expand(const QString& tpl, const QString& selection,
                            const QString& filePath) {
    QMap<QString, QString> v;
    v["selection"] = selection;
    v["file"] = filePath;
    v["filename"] = QFileInfo(filePath).fileName();
    v["lang"] = QFileInfo(filePath).suffix().toLower();
    return expand(tpl, v);
}
