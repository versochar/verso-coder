#pragma once
#include <QString>

// .editorconfig dosyasını okuyup verilen dosya için geçerli ayarı üretir.
// Desteklenen anahtarlar: indent_style, indent_size, tab_width, end_of_line,
// charset, trim_trailing_whitespace, insert_final_newline, root.
struct EditorConfig {
    bool found = false;
    bool useSpaces = true;
    int indentSize = 4;
    QString endOfLine = "lf"; // "lf" | "crlf"
    bool trimTrailing = false;
    bool insertFinalNewline = false;
};

class EditorConfigParser {
public:
    static EditorConfig forFile(const QString& filePath);

private:
    static bool patternMatches(const QString& pattern, const QString& relPath);
};
