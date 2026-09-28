// Verso Coder mağaza eklentisi: HTML denetleme + iskelet
// Yığın tabanlı etiket eşleştirme (void öğeler atlanır), HTML5 iskelet üretir.
// @name HTML Araçları
// @version 1.0.0
// @permission fs.read
// @permission fs.write
// @permission ui
(function () {
    var BOS = { meta: 1, link: 1, br: 1, hr: 1, img: 1, input: 1, source: 1,
                track: 1, wbr: 1, embed: 1, param: 1, col: 1, base: 1 };
    function dogrula(text) {
        var yigin = [], hata = [], lines = text.split("\n"), i, m;
        var re = /<\/?([A-Za-z][A-Za-z0-9]*)[^>]*?>/g;
        for (i = 0; i < lines.length; i++) {
            re.lastIndex = 0;
            while ((m = re.exec(lines[i])) !== null) {
                var ad = m[1].toLowerCase(), kapanis = m[0].charAt(1) === "/";
                if (BOS[ad] || /\/\s*>$/.test(m[0])) continue;
                if (!kapanis) {
                    yigin.push({ ad: ad, line: i + 1 });
                } else if (yigin.length && yigin[yigin.length - 1].ad === ad) {
                    yigin.pop();
                } else {
                    hata.push({ line: i + 1, text: "eşleşmeyen </" + ad + ">" });
                }
            }
        }
        for (var j = 0; j < yigin.length; j++)
            hata.push({ line: yigin[j].line, text: "kapanmayan <" + yigin[j].ad + ">" });
        return hata;
    }
    function kaynak(arg) {
        if (arg || arg === "") return { path: "", text: arg };
        var p = verso.currentFile();
        if (!p) { verso.showStatus("Önce bir dosya açın", 3000); return null; }
        return { path: p, text: verso.readFile(p) };
    }
    verso.registerCommand("dogrula", "HTML Doğrula", function (arg) {
        var k = (arg || arg === "") ? { path: "", text: arg } : kaynak();
        if (!k) return "dosya yok";
        var h = dogrula(k.text || "");
        if (k.path) {
            var prob = [], i;
            for (i = 0; i < h.length; i++)
                prob.push({ file: k.path, line: h[i].line, message: h[i].text });
            verso.reportProblems(JSON.stringify(prob));
        }
        var s = h.length ? h.length + " hata" : "geçerli";
        verso.showStatus(s, 4000);
        return s;
    });
    verso.registerCommand("iskelet", "HTML İskelet", function (arg) {
        var baslik = arg || verso.inputBox("Başlık:", "Sayfa");
        if (!baslik) return "vazgeçildi";
        var t = "<!DOCTYPE html>\n<html lang=\"tr\">\n<head>\n" +
            "<meta charset=\"utf-8\">\n" +
            "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n" +
            "<title>" + baslik + "</title>\n</head>\n<body>\n\n</body>\n</html>\n";
        if (!arg) {
            var p = verso.currentFile();
            if (!p) return "dosya yok";
            if ((verso.readFile(p) || "").replace(/^\s+|\s+$/g, "") !== "") {
                verso.showStatus("dosya boş değil", 3000);
                return "dosya boş değil";
            }
            verso.writeFile(p, t);
            verso.showStatus("iskelet yazıldı", 3000);
        }
        return t;
    });
    verso.log("html araçları yüklendi");
})();
