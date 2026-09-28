// Verso Coder mağaza eklentisi: JSON doğrulama + düzenleme
// @name JSON Bakımı
// @version 1.0.0
// @permission fs.read
// @permission fs.write
// @permission ui
(function () {
    function oku(arg) {
        if (arg) return arg;
        var p = verso.currentFile();
        if (!p) { verso.showStatus("Önce bir dosya açın", 3000); return null; }
        return verso.readFile(p);
    }
    verso.registerCommand("dogrula", "JSON Doğrula", function (arg) {
        var t = oku(arg);
        if (t === null) return "dosya yok";
        try {
            JSON.parse(t);
            verso.showStatus("Geçerli JSON", 3000);
            return "geçerli";
        } catch (e) {
            verso.showStatus("JSON hatası: " + e.message, 5000);
            return "hata: " + e.message;
        }
    });
    verso.registerCommand("duzenle", "JSON Düzenle (pretty)", function (arg) {
        var t = oku(arg);
        if (t === null) return "dosya yok";
        var o;
        try {
            o = JSON.parse(t);
        } catch (e) {
            return "hata: " + e.message;
        }
        var duzgun = JSON.stringify(o, null, 2);
        if (!arg) {
            verso.writeFile(verso.currentFile(), duzgun);
            verso.showStatus("JSON düzenlendi", 3000);
        }
        return duzgun;
    });
    verso.log("json bakımı yüklendi");
})();
