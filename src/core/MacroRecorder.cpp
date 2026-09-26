#include "MacroRecorder.h"

bool MacroRecorder::isMacroCmd(const QString& id) {
    return id == "macro.record" || id == "macro.play" || id == "macro.clear";
}

void MacroRecorder::push(const QString& cmdId) {
    if (!m_on || cmdId.isEmpty() || isMacroCmd(cmdId)) return;
    if (!m_rec.isEmpty() && m_rec.last() == cmdId) return; // art arda kopya yok
    m_rec << cmdId;
    if (m_rec.size() > 200) m_rec.removeFirst();
}

void MacroRecorder::save() const {
    QSettings("Verso", "VersoCoder").setValue("macro/last", m_rec);
}

void MacroRecorder::load() {
    m_rec = QSettings("Verso", "VersoCoder").value("macro/last").toStringList().mid(0, 200);
}
