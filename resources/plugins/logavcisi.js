// Verso Coder mağaza eklentisi: hata ayıklama artığı avcısı
// console.log / print( / qDebug( / dbg!( satırlarını Problems'e döker,
// onayla toplu siler.
// @name Log Avcısı
// @version 1.0.0
// @permission fs.read
// @permission fs.write
// @permission ui
(function () {
    function bul(text) {
        var out = [], lines = text.split("\n"), i;
        for (i = 0; i < lines.length; i++) {
            if (/^\s*(console\.(log|debug|warn|error)\s*\(|print\s*\(|qDebug\s*\(|dbg!\s*\()/.test(lines[i]))
                out.push({ line: i + 1, text: lines[i].replace(/^\s+/, "") });
        }
        return out;
    }
    function kaynak(arg) {
        if (arg) return { path: "", text: arg };
        var p = verso.currentFile();
        if (!p) { verso.showStatus("Önce bir dosya açın", 3000); return null; }
        return { path: p, text: verso.readFile(p) };
    }
    verso.registerCommand("tara", "Log Artığı Tara", function (arg) {
        var k = kaynak(arg);
        if (!k) return "dosya yok";
        var b = bul(k.text);
        if (k.path) {
            var prob = [], i;
            for (i = 0; i < b.length; i++)
                prob.push({ file: k.path, line: b[i].line, message: "artık: " + b[i].text });
            verso.reportProblems(JSON.stringify(prob));
        }
        var s = b.length + " artık satır";
        verso.showStatus(s, 4000);
        return s;
    });
    verso.registerCommand("temizle", "Log Artığı Temizle", function (arg) {
        var k = kaynak(arg);
        if (!k) return "dosya yok";
        var lines = k.text.split("\n"), tut = [], sil = 0, i;
        for (i = 0; i < lines.length; i++) {
            if (/^\s*(console\.(log|debug|warn|error)\s*\(|print\s*\(|qDebug\s*\(|dbg!\s*\()/.test(lines[i])) sil++;
            else tut.push(lines[i]);
        }
        if (!arg) {
            verso.writeFile(k.path, tut.join("\n"));
            verso.showStatus(sil + " satır silindi", 3000);
        }
        return sil + " satır";
    });
    verso.log("log avcısı yüklendi");
})();
