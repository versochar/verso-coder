// Verso Coder mağaza eklentisi: import sıralayıcı (Python + JS/TS)
// Ardışık import bloklarını alfabetik sıralar.
// @name Import Düzenleyici
// @version 1.0.0
// @permission fs.read
// @permission fs.write
// @permission ui
(function () {
    function blokMu(s) {
        return /^\s*(import\s|from\s+\S+\s+import\s)/.test(s) ||  // python
               /^\s*import\s/.test(s);                            // js/ts
    }
    function duzenle(text) {
        var lines = text.split("\n"), out = [], i = 0, blok, basla, n = 0;
        while (i < lines.length) {
            if (!blokMu(lines[i])) { out.push(lines[i]); i++; continue; }
            basla = i; blok = [];
            while (i < lines.length && blokMu(lines[i])) { blok.push(lines[i]); i++; }
            if (blok.length > 1) {
                var sirali = blok.slice().sort(), degisti = false;
                for (var k = 0; k < blok.length; k++)
                    if (blok[k] !== sirali[k]) { degisti = true; break; }
                if (degisti) { blok = sirali; n++; }
            }
            for (var j = 0; j < blok.length; j++) out.push(blok[j]);
        }
        return { text: out.join("\n"), blok: n };
    }
    verso.registerCommand("duzenle", "Import Sırala", function (arg) {
        var p = "", t = arg;
        if (!t) {
            p = verso.currentFile();
            if (!p) { verso.showStatus("Önce bir dosya açın", 3000); return "dosya yok"; }
            t = verso.readFile(p);
        }
        var r = duzenle(t || "");
        if (!arg) {
            verso.writeFile(p, r.text);
            verso.showStatus(r.blok + " blok sıralandı", 3000);
        }
        return r.blok + " blok";
    });
    verso.log("import düzenleyici yüklendi");
})();
