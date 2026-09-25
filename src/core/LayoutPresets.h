#pragma once
#include <QStringList>

// Stage 10: yerleşim ön ayarları — arayüz bileşenlerinin görünürlük şablonları.
struct LayoutState {
    bool menuBar = true;
    bool activityBar = true;
    bool sidePanel = true;
    bool statusBar = true;
    bool bottomPanel = false; // terminal dock
    bool minimap = true;
    bool breadcrumb = true;
};

class LayoutPresets {
public:
    static QStringList presetNames();               // {"standart", "editor", "zen"}
    static QString presetTitle(const QString& name);
    static bool isValid(const QString& name);
    static LayoutState preset(const QString& name); // bilinmeyen → standart
};
