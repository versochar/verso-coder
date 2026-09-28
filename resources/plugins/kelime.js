// Verso Coder örnek eklentisi: kelime sayacı (Stage 41 referansı)
// Yalnız "fs.read" + "ui" izni ister: açık dosyanın yolunu ana taraftan alır,
// dosya KÖK İÇİNDEYSE okur, sonucu durum çubuğunda gösterir.
// @name Kelime Sayacı
// @version 1.0.0
// @permission fs.read
// @permission ui
(function () {
    verso.log("kelime sayacı yüklendi");
    verso.registerCommand("say", "Kelime Say (açık dosya)", function () {
        var path = verso.currentFile();
        if (!path) {
            verso.showStatus("Önce bir dosya açın", 3000);
            return "dosya yok";
        }
        var text = verso.readFile(path);
        if (!text) {
            verso.showStatus("Okunamadı (kök dışı ya da izin yok)", 3000);
            return "okunamadı";
        }
        var words = text.split(/\s+/).filter(function (w) { return w.length > 0; }).length;
        var lines = text.split("\n").length;
        verso.showStatus(words + " kelime · " + lines + " satır", 5000);
        return words + " kelime";
    });
})();
