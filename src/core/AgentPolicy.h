#pragma once
#include <QString>
#include <QStringList>

// Stage 34: ajan politika motoru. Saf mantık — ağ/UI yok, doğrudan test edilir.
// Kural: her yazma/komut aracı (a) kök hapsedmeye tabidir, (b) onay ister,
// (c) otonom modda serbest bırakılmaz.
struct AgentPolicy {
    bool autonomous = false;   // otonom mod (varsayılan KAPALI)
    bool allowWrite = false;   // write_file
    bool allowCommand = false; // run_command
    bool allowTests = false;   // run_tests
    int maxSteps = 5;
    bool requireApproval = true;

    // Araç hiçbir koşulda serbest olan salt-okunur küme.
    static bool isReadOnlyTool(const QString& name);
    // Araç durumu değiştiriyor mu (onay gerekir).
    static bool isMutatingTool(const QString& name);
    static QStringList readOnlyTools();

    // Bu politikada araç çalıştırılabilir mi?
    bool toolAllowed(const QString& name) const;
    // Onay gerekiyor mu? (yalnız mutating araçlar)
    bool needsApproval(const QString& name) const;
    // Otonom modda yalnız okuma + hedef raporlamaya izin verilir.
    bool autonomousAllows(const QString& name) const;

    QString summary() const;
    // Güvenli varsayılan: yalnız okuma, yazma/komut kapalı, onay zorunlu.
    static AgentPolicy safeDefault();
    // Otonom ama salt-okunur: denetim/raporlama hedefleri için.
    static AgentPolicy autonomousReadOnly(int steps = 4);
};
