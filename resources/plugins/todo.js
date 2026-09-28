// Verso Coder mağaza eklentisi: TODO/FIXME paneli
// Kaydetmede otomatik tarar, Problems'e döker.
// @name TODO Paneli
// @version 1.0.0
// @permission fs.read
// @permission ui
// @permission events
(function () {
    function bul(text) {
        var out = [], lines = text.split("\n"), i, m;
        for (i = 0; i < lines.length; i++) {
            m = lines[i].match(/(TODO|FIXME|HACK|XXX)\s*:?\s*(.*)$/);
            if (m) out.push({ line: i + 1, tag: m[1], text: m[2].replace(/\s+$/, "") });
        }
        return out;
    }
    function tara(path, text) {
        var b = bul(text), prob = [], i;
        if (path) {
            for (i = 0; i < b.length; i++)
                prob.push({ file: path, line: b[i].line,
                            message: b[i].tag + ": " + b[i].text });
            verso.reportProblems(JSON.stringify(prob));
        }
        return b.length + " yapılacak";
    }
    verso.registerCommand("tara", "TODO Tara", function (arg) {
        if (arg) return tara("", arg);
        var p = verso.currentFile();
        if (!p) { verso.showStatus("Önce bir dosya açın", 3000); return "dosya yok"; }
        var s = tara(p, verso.readFile(p));
        verso.showStatus(s, 3000);
        return s;
    });
    verso.onEvent("save", function (p) {
        if (!p) return;
        try { tara(p, verso.readFile(p)); } catch (e) {}
    });
    verso.log("todo paneli yüklendi");
})();
