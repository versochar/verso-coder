// Verso Coder mağaza eklentisi: var -> const/let
// Yeniden atanan let olur, atanmayan const olur.
// @name JS var Dönüştürücü
// @version 1.0.0
// @permission fs.read
// @permission fs.write
// @permission ui
(function () {
    verso.registerCommand("donustur", "var Dönüştür", function (arg) {
        var p = "", t = arg;
        if (!t && t !== "") {
            p = verso.currentFile();
            if (!p) { verso.showStatus("Önce bir dosya açın", 3000); return "dosya yok"; }
            t = verso.readFile(p);
        }
        t = t || "";
        var lines = t.split("\n"), i, m, adlar = [];
        for (i = 0; i < lines.length; i++) {
            m = lines[i].match(/^\s*var\s+([A-Za-z_$][\w$]*)\s*=/);
            if (m) adlar.push({ ad: m[1], satir: i });
        }
        var c = 0, l = 0;
        for (var j = 0; j < adlar.length; j++) {
            var a = adlar[j], yeniden = false;
            var yaz = new RegExp("\\b" + a.ad + "\\s*=[^=]");
            for (i = 0; i < lines.length; i++) {
                if (i === a.satir) continue;
                if (yaz.test(lines[i])) { yeniden = true; break; }
            }
            lines[a.satir] = lines[a.satir].replace(/^\s*var\s+/,
                lines[a.satir].match(/^\s*/)[0] + (yeniden ? "let " : "const "));
            if (yeniden) l++; else c++;
        }
        if (!arg && arg !== "") {
            verso.writeFile(p, lines.join("\n"));
            verso.showStatus(c + " const, " + l + " let", 3000);
        }
        return c + " const · " + l + " let";
    });
    verso.log("js var dönüştürücü yüklendi");
})();
