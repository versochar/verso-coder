// Verso Coder mağaza eklentisi: C başlık koruma muhafızı
// Dosya adından muhafız üretir (#ifndef X_H ... #endif); varsa dokunmaz.
// @name C Header Guard
// @version 1.0.0
// @permission fs.read
// @permission fs.write
// @permission ui
(function () {
    function muhafiz(path) {
        var ad = "KORUMA_H";
        if (path) {
            var b = path.replace(/\\/g, "/").split("/");
            ad = b[b.length - 1].toUpperCase().replace(/[^A-Z0-9]/g, "_");
            if (!/[A-Z0-9]$/.test(ad)) ad += "_H";
            else if (!/_H$/.test(ad)) ad += "_H";
        }
        return ad;
    }
    verso.registerCommand("ekle", "Header Guard Ekle", function (arg) {
        var p = "", t = arg;
        if (!t && t !== "") {
            p = verso.currentFile();
            if (!p) { verso.showStatus("Önce bir dosya açın", 3000); return "dosya yok"; }
            t = verso.readFile(p);
        }
        t = t || "";
        if (/#\s*ifndef\b/.test(t)) return "zaten var";
        var g = muhafiz(p);
        var cikti = "#ifndef " + g + "\n#define " + g + "\n\n" +
            t.replace(/^\s+/, "") + "\n#endif // " + g + "\n";
        if (!arg && arg !== "") {
            verso.writeFile(p, cikti);
            verso.showStatus("muhafız eklendi: " + g, 3000);
        }
        return g;
    });
    verso.log("c header guard yüklendi");
})();
