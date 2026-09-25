#pragma once
#include <QJsonObject>
#include <QList>
#include <QString>

// Stage 13: metin düzenlemeleri — TextEdit[] / WorkspaceEdit → uygula + önizleme.
struct LspTextEdit {
    int startLine = 0;
    int startCol = 0;
    int endLine = 0;
    int endCol = 0;
    QString newText;
    QString file; // WorkspaceEdit'ten geliyorsa dolu
};

class TextEdits {
public:
    // textDocument/formatting yanıtı: TextEdit[]
    static QList<LspTextEdit> parse(const QJsonObject& res);
    static QList<LspTextEdit> parseArray(const class QJsonArray& arr,
                                        const QString& file = QString());
    // WorkspaceEdit yanıtı: {changes:{uri:[...]}, documentChanges:[...]}
    static QList<LspTextEdit> parseWorkspace(const QJsonObject& res);
    // Düzenlemeleri metne uygula (sondan başa, çakışmasız varsayımı)
    static QString apply(const QString& text, QList<LspTextEdit> edits);
    // Önizleme satırları: "satır: eski → yeni"
    static QStringList preview(const QString& text, const QList<LspTextEdit>& edits,
                              int context = 1);
};
