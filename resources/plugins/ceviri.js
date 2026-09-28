// Verso Coder mağaza eklentisi: EN<->TR çeviri (ücretsiz API)
// Türkçe harf varsa TR->EN, yoksa EN->TR.
// @name Çeviri
// @version 1.0.0
// @permission net
// @permission ui
(function () {
    verso.registerCommand("cevir", "Çevir (EN-TR)", function (arg) {
        var t = arg || verso.inputBox("Çevrilecek metin?", "");
        if (!t) return "vazgeçildi";
        var yon = /[çğıöşüÇĞİÖŞÜ]/.test(t) ? "tr|en" : "en|tr";
        var url = "https://api.mymemory.translated.net/get?q=" +
            encodeURIComponent(t) + "&langpair=" + yon;
        var ham;
        try {
            ham = verso.fetch(url, 12000);
        } catch (e) {
            return "ağ hatası";
        }
        if (!ham) return "yanıt yok (kota dolmuş olabilir)";
        try {
            var o = JSON.parse(ham);
            var c = o.responseData && o.responseData.translatedText;
            if (!c) return "çevrilemedi";
            verso.showStatus(c, 8000);
            return c;
        } catch (e) {
            return "çözümlenemedi";
        }
    });
    verso.log("çeviri yüklendi");
})();
