// Verso Coder mağaza eklentisi: Java getter/setter üretici
// "private Tür ad;" alanlarını bulur, sınıfın son } öncesine ekler.
// @name Java Getter Setter
// @version 1.0.0
// @permission fs.read
// @permission fs.write
// @permission ui
(function () {
    function buyuk(s) { return s.charAt(0).toUpperCase() + s.slice(1); }
    verso.registerCommand("uret", "Getter/Setter Üret", function (arg) {
        var p = "", t = arg;
        if (!t && t !== "") {
            p = verso.currentFile();
            if (!p) { verso.showStatus("Önce bir dosya açın", 3000); return "dosya yok"; }
            t = verso.readFile(p);
        }
        t = t || "";
        var re = /^\s*private\s+([\w<>\[\]]+)\s+(\w+)\s*;/gm, m, alan = [];
        while ((m = re.exec(t)) !== null) alan.push({ tur: m[1], ad: m[2] });
        if (!alan.length) return "alan yok";
        var kod = [], i;
        for (i = 0; i < alan.length; i++) {
            var a = alan[i], b = buyuk(a.ad);
            kod.push("    public " + a.tur + " get" + b + "() { return " + a.ad + "; }");
            kod.push("    public void set" + b + "(" + a.tur + " " + a.ad +
                     ") { this." + a.ad + " = " + a.ad + "; }");
        }
        var kes = t.lastIndexOf("}");
        var cikti = kes >= 0
            ? t.slice(0, kes) + "\n" + kod.join("\n") + "\n" + t.slice(kes)
            : t + "\n" + kod.join("\n") + "\n";
        if (!arg && arg !== "") {
            verso.writeFile(p, cikti);
            verso.showStatus(alan.length + " alan üretildi", 3000);
        }
        return alan.length + " alan";
    });
    verso.log("java getter setter yüklendi");
})();
