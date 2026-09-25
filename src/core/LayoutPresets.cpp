#include "LayoutPresets.h"

QStringList LayoutPresets::presetNames() {
    return {"standart", "editor", "zen"};
}

QString LayoutPresets::presetTitle(const QString& name) {
    if (name == "editor") return "Editör";
    if (name == "zen")    return "Zen";
    return "Standart";
}

bool LayoutPresets::isValid(const QString& name) {
    return presetNames().contains(name);
}

LayoutState LayoutPresets::preset(const QString& name) {
    LayoutState s; // standart varsayılanı
    if (name == "editor") {
        s.sidePanel = false;
        s.bottomPanel = false;
    } else if (name == "zen") {
        s.menuBar = false;
        s.activityBar = false;
        s.sidePanel = false;
        s.statusBar = false;
        s.bottomPanel = false;
        s.minimap = false;
        s.breadcrumb = false;
    }
    return s;
}
