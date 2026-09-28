// Verso Coder mağaza eklentisi: çalışma alanı notları (dosyasız)
// Notlar workspace'e saklanır; proje değişince ayrı liste gelir.
// @name Çalışma Alanı Notları
// @version 1.0.0
// @permission ui
(function () {
    function al() {
        var ham = verso.getWorkspaceState("notlar");
        return ham ? ham.split("\n") : [];
    }
    function kaydet(a) {
        verso.setWorkspaceState("notlar", a.join("\n"));
    }
    function sira(a) {
        var s = [], i;
        for (i = 0; i < a.length; i++) s.push((i + 1) + ". " + a[i]);
        return s;
    }
    verso.registerCommand("ekle", "Not Ekle", function (arg) {
        var t = arg || verso.inputBox("Not:", "");
        if (!t) return "vazgeçildi";
        var a = al();
        a.push(t);
        kaydet(a);
        verso.showStatus(a.length + " not", 2000);
        return "eklendi";
    });
    verso.registerCommand("listele", "Notları Listele", function () {
        var a = al();
        if (!a.length) return "not yok";
        return sira(a).join("\n");
    });
    verso.registerCommand("sil", "Not Sil", function (arg) {
        var a = al();
        if (!a.length) return "not yok";
        var i = parseInt(arg, 10);
        if (!(i >= 1 && i <= a.length)) {
            var sec = verso.quickPick(sira(a), "Sil:");
            if (!sec) return "vazgeçildi";
            i = parseInt(sec, 10);
        }
        if (!(i >= 1 && i <= a.length)) return "vazgeçildi";
        a.splice(i - 1, 1);
        kaydet(a);
        return "silindi";
    });
    verso.log("çalışma alanı notları yüklendi");
})();
