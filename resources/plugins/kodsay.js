// Verso Coder mağaza eklentisi: kod istatistiği
// Satır / kod / yorum / boş + kaba işlev sayımı.
// @name Kod Sayacı
// @version 1.0.0
// @permission fs.read
// @permission ui
(function () {
    verso.registerCommand("istatistik", "Kod İstatistiği", function (arg) {
        var t = arg;
        if (!t) {
            var p = verso.currentFile();
            if (!p) { verso.showStatus("Önce bir dosya açın", 3000); return "dosya yok"; }
            t = verso.readFile(p);
        }
        if (!t) return "boş";
        var lines = t.split("\n"), bos = 0, yorum = 0, islev = 0, i, s;
        for (i = 0; i < lines.length; i++) {
            s = lines[i].replace(/^\s+/, "");
            if (s === "") { bos++; continue; }
            if (/^(\/\/|#|\*|\/\*|<!--)/.test(s)) { yorum++; continue; }
            if (/\bfunction\b|=>|^\s*(def|fn|func)\b/.test(lines[i])) islev++;
        }
        var sonuc = lines.length + " satır · " + (lines.length - bos - yorum) +
            " kod · " + yorum + " yorum · " + bos + " boş · ~" + islev + " işlev";
        verso.showStatus(sonuc, 5000);
        return sonuc;
    });
    verso.log("kod sayacı yüklendi");
})();
