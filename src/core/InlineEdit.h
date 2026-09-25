#pragma once
#include <QString>

// Stage 15: satır içi yeniden yazma — tarif + seçim → istem, yanıttan kod çıkar.
class InlineEdit {
public:
    static QString buildPrompt(const QString& instruction, const QString& code,
                               const QString& lang);
    // Son ``` bloğunu çıkar (yoksa tüm yanıtı kırpılmış döndür)
    static QString extractCode(const QString& reply);
};
