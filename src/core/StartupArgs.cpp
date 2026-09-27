#include "StartupArgs.h"

StartupOptions StartupArgs::parse(const QStringList& args) {
    StartupOptions o;
    for (int i = 0; i < args.size(); ++i) {
        const QString a = args[i];
        if (a == "--help" || a == "-h") { o.help = true; continue; }
        if (a == "--version" || a == "-v") { o.version = true; continue; }
        if (a == "--new" || a == "-n") { o.isNew = true; continue; }
        if (a == "--wait" || a == "-w") { o.wait = true; continue; }
        if (a == "--no-restore") { o.noRestore = true; continue; }
        if (a == "--portable") { o.portable = true; continue; }
        if (a == "--line" || a == "-l") {
            if (i + 1 < args.size()) o.line = args[++i].toInt();
            continue;
        }
        if (a.startsWith("--line=")) { o.line = a.mid(7).toInt(); continue; }
        if (a.startsWith("-l") && a.size() > 2) { o.line = a.mid(2).toInt(); continue; }
        if (a == "--command" || a == "-c") {
            if (i + 1 < args.size()) o.command = args[++i];
            continue;
        }
        if (a.startsWith("--command=")) { o.command = a.mid(10); continue; }
        if (a.startsWith("--")) continue;   // bilinmeyen uzun seçenek: yoksay
        o.paths << a;                        // konumsal: dosya/klasör
    }
    return o;
}

QString StartupArgs::helpText() {
    return QString(
        "Verso Coder\n"
        "Kullanım: verso-coder [seçenekler] [dosya|klasör ...]\n\n"
        "Seçenekler:\n"
        "  -n, --new           Boş pencere aç (oturumu geri yükleme)\n"
        "  -l, --line N        Dosyayı N. satırda aç\n"
        "  -c, --command ID    Açılışta verilen komutu çalıştır\n"
        "  -w, --wait          Kapanana kadar bekle (tek örnek ile)\n"
        "      --no-restore    Oturum geri yüklemeyi kapat\n"
        "      --portable      Taşınabilir kip (ayarlar uygulama yanında)\n"
        "  -h, --help          Bu yardımı göster\n"
        "  -v, --version       Sürümü göster\n");
}
