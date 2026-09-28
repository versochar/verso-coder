// Verso Coder mağaza eklentisi: tek satırlık function -> ok işlevi
// @name JS Arrow Dönüştürücü
// @version 1.0.0
// @permission fs.read
// @permission fs.write
// @permission ui
(function () {
    verso.registerCommand("donustur", "Arrow Dönüştür", function (arg) {
        var p = "", t = arg;
        if (!t && t !== "") {
            p = verso.currentFile();
            if (!p) { verso.showStatus("Önce bir dosya açın", 3000); return "dosya yok"; }
            t = verso.readFile(p);
        }
        t = t || "";
        var n = 0;
        t = t.replace(/function\s*\(([^)]*)\)\s*\{\s*return\s+([^;]+);\s*\}/g,
            function () {
                n++;
                var args = arguments[1].replace(/^\s+|\s+$/g, "");
                var govde = arguments[2].replace(/^\s+|\s+$/g, "");
                return "(" + args + ") => " + govde;
            });
        if (!arg && arg !== "") {
            verso.writeFile(p, t);
            verso.showStatus(n + " işlev çevrildi", 3000);
        }
        return n + " işlev";
    });
    verso.log("js arrow dönüştürücü yüklendi");
})();
