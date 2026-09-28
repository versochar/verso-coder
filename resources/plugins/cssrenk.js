// Verso Coder mağaza eklentisi: CSS renklerini :root değişkenine çıkarır
// @name CSS Renk Değişkeni
// @version 1.0.0
// @permission fs.read
// @permission fs.write
// @permission ui
(function () {
    verso.registerCommand("degisken", "Renkleri Değişkene Çıkar", function (arg) {
        var p = "", t = arg;
        if (!t && t !== "") {
            p = verso.currentFile();
            if (!p) { verso.showStatus("Önce bir dosya açın", 3000); return "dosya yok"; }
            t = verso.readFile(p);
        }
        t = t || "";
        var bulunan = [], re = /#([0-9a-fA-F]{3}|[0-9a-fA-F]{6})\b/g, m;
        while ((m = re.exec(t)) !== null) {
            var r = m[0].toLowerCase();
            if (bulunan.indexOf(r) < 0) bulunan.push(r);
        }
        if (!bulunan.length) return "renk yok";
        var kok = [":root {"], i;
        for (i = 0; i < bulunan.length; i++)
            kok.push("  --renk-" + (i + 1) + ": " + bulunan[i] + ";");
        kok.push("}");
        var cikti = kok.join("\n") + "\n\n" + t;
        for (i = 0; i < bulunan.length; i++) {
            var degs = "--renk-" + (i + 1);
            cikti = cikti.split(bulunan[i]).join("var(" + degs + ")");
            // :root bloğundaki tanımı geri al
            cikti = cikti.split(degs + ": var(" + degs + ")").join(degs + ": " + bulunan[i]);
        }
        if (!arg && arg !== "") {
            verso.writeFile(p, cikti);
            verso.showStatus(bulunan.length + " renk değişkende", 3000);
        }
        return bulunan.length + " renk";
    });
    verso.log("css renk değişkeni yüklendi");
})();
