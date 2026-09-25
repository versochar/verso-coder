#include "HoverCard.h"
#include <QStringList>

QString HoverCard::bodyToHtml(const QString& body, const Colors& c) {
    QString html;
    bool inCode = false;
    const QStringList lines = body.split('\n');
    for (const QString& ln : lines) {
        if (ln.trimmed().startsWith("```")) {
            if (inCode) { html += "</pre>"; inCode = false; }
            else {
                html += QString("<pre style='background:%1;padding:6px;border-radius:4px;'>")
                            .arg(c.codeBg);
                inCode = true;
            }
            continue;
        }
        if (inCode)
            html += QString("<font color='%1'>%2</font>\n").arg(c.code, ln.toHtmlEscaped());
        else if (!ln.trimmed().isEmpty())
            html += ln.toHtmlEscaped() + "<br>";
    }
    if (inCode) html += "</pre>";
    return html;
}

QString HoverCard::render(const QString& title, const QString& body, const Colors& c) {
    QString html = QString(
        "<div style='background:%1;border:1px solid %2;border-radius:6px;min-width:280px;max-width:520px;'>"
        "<div style='padding:6px 10px;border-bottom:1px solid %2;'>"
        "<b><font color='%3'>%4</font></b></div>"
        "<div style='padding:8px 10px;'><font color='%5'>%6</font></div></div>")
        .arg(c.bg, c.border, c.title, title.toHtmlEscaped(), c.text,
             bodyToHtml(body, c));
    return html;
}
