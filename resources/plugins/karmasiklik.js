// Verso Coder mağaza eklentisi: kaba karmaşıklık ölçer
// function/def başlıklarını izler, dallanma anahtarlarını sayar,
// skoru 5'i aşanları Problems'e yazar.
// @name Karmaşıklık Ölçer
// @version 1.0.0
// @permission fs.read
// @permission ui
(function () {
    function olc(text) {
        var lines = text.split("\n"), cur = "(global)", skor = { "(global)": 1 },
            i, m;
        for (i = 0; i < lines.length; i++) {
            m = lines[i].match(/^\s*(function\s+(\w+)|def\s+(\w+)|(\w+)\s*=\s*\(.*\)\s*=>)/);
            if (m) {
                cur = m[2] || m[3] || m[4] || "(anonim)";
                if (!(cur in skor)) skor[cur] = 1;
                continue;
            }
            var k = lines[i].match(/\b(if|for|while|catch|case|&&|\|\|)\b/g);
            if (k) skor[cur] = (skor[cur] || 1) + k.length;
        }
        return skor;
    }
    verso.registerCommand("olc", "Karmaşıklık Ölç", function (arg) {
        var p = "", t = arg;
        if (!t) {
            p = verso.currentFile();
            if (!p) { verso.showStatus("Önce bir dosya açın", 3000); return "dosya yok"; }
            t = verso.readFile(p);
        }
        var skor = olc(t || ""), ad, risk = [], toplam = 0;
        for (ad in skor) {
            toplam += skor[ad];
            if (skor[ad] > 5) risk.push(ad + "(" + skor[ad] + ")");
        }
        if (p && risk.length) {
            var prob = [], i;
            for (i = 0; i < risk.length; i++)
                prob.push({ file: p, line: 1, message: "karmaşık: " + risk[i] });
            verso.reportProblems(JSON.stringify(prob));
        }
        var s = "skor: " + toplam + (risk.length ? " · riskli: " + risk.join(", ") : " · temiz");
        verso.showStatus(s, 5000);
        return s;
    });
    verso.log("karmaşıklık ölçer yüklendi");
})();
