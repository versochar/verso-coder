// Verso Coder mağaza eklentisi: adli kod parçacıkları
// Parçalar globalState'te JSON olarak durur. "kaydet" biçimi: "ad:::içerik".
// @name Snippet Kitaplığı
// @version 1.0.0
// @permission fs.read
// @permission fs.write
// @permission ui
(function () {
    function al() {
        try {
            var o = JSON.parse(verso.getGlobalState("parcalar") || "{}");
            return (typeof o === "object" && o) ? o : {};
        } catch (e) { return {}; }
    }
    function kaydet(o) {
        verso.setGlobalState("parcalar", JSON.stringify(o));
    }
    verso.registerCommand("kaydet", "Snippet Kaydet", function (arg) {
        var ad = arg, icerik = "";
        if (arg && arg.indexOf(":::") >= 0) {
            ad = arg.slice(0, arg.indexOf(":::"));
            icerik = arg.slice(arg.indexOf(":::") + 3);
        } else {
            ad = verso.inputBox("Ad:", "");
            if (!ad) return "vazgeçildi";
            icerik = verso.inputBox("İçerik:", "");
        }
        if (!ad || !icerik) return "vazgeçildi";
        var o = al();
        o[ad] = icerik;
        kaydet(o);
        return "kaydedildi: " + ad;
    });
    verso.registerCommand("liste", "Snippet Listesi", function () {
        var o = al(), ad, s = [];
        for (ad in o) s.push(ad);
        return s.length ? s.join("\n") : "parça yok";
    });
    verso.registerCommand("ekle", "Snippet Ekle", function (arg) {
        var o = al(), ad = arg;
        if (!ad || !o[ad]) {
            var s = [];
            for (var k in o) s.push(k);
            if (!s.length) return "parça yok";
            ad = verso.quickPick(s, "Parça:");
            if (!ad) return "vazgeçildi";
        }
        if (!o[ad]) return "bulunamadı";
        var p = verso.currentFile();
        if (!p) { verso.showStatus("Önce bir dosya açın", 3000); return "dosya yok"; }
        var eski = verso.readFile(p) || "";
        verso.writeFile(p, eski + (eski ? "\n" : "") + o[ad] + "\n");
        verso.showStatus("eklendi: " + ad, 3000);
        return "eklendi: " + ad;
    });
    verso.log("snippet kitaplığı yüklendi");
})();
