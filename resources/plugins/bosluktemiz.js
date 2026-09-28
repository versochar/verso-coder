// Verso Coder mağaza eklentisi: sondaki boşluk temizliği
// Kaydetmede otomatik çalışır.
// @name Boşluk Temizleyici
// @version 1.0.0
// @permission fs.read
// @permission fs.write
// @permission ui
// @permission events
(function () {
    function temizle(text) {
        var lines = text.split("\n"), n = 0, i;
        for (i = 0; i < lines.length; i++) {
            var s = lines[i].replace(/[ \t]+$/, "");
            if (s !== lines[i]) n++;
            lines[i] = s;
        }
        while (lines.length > 1 && lines[lines.length - 1] === "" &&
               lines[lines.length - 2] === "") lines.pop();
        return { text: lines.join("\n"), satir: n };
    }
    function uygula(path, text, yaz) {
        var r = temizle(text);
        if (yaz) verso.writeFile(path, r.text);
        return r.satir + " satır";
    }
    verso.registerCommand("temizle", "Boşluk Temizle", function (arg) {
        if (arg) return uygula("", arg, false);
        var p = verso.currentFile();
        if (!p) { verso.showStatus("Önce bir dosya açın", 3000); return "dosya yok"; }
        var s = uygula(p, verso.readFile(p), true);
        verso.showStatus(s, 3000);
        return s;
    });
    verso.onEvent("save", function (p) {
        if (!p) return;
        try { verso.writeFile(p, temizle(verso.readFile(p)).text); } catch (e) {}
    });
    verso.log("boşluk temizleyici yüklendi");
})();
