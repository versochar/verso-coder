// Verso Coder mağaza eklentisi: hızlı GET denetimi
// Motor yalnız GET destekler; yanıt HTML görünümde gösterilir.
// @name REST İstemcisi
// @version 1.0.0
// @permission net
// @permission ui
(function () {
    function esc(s) {
        return s.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;");
    }
    verso.registerCommand("istek", "GET İsteği", function (arg) {
        var url = arg || verso.inputBox("URL (GET):", "https://");
        if (!url) return "vazgeçildi";
        if (!/^https?:\/\//.test(url)) return "hata: http(s) olmalı";
        var ham = verso.fetch(url, 15000);
        if (!ham) return "yanıt yok";
        var govde = ham.length > 20000 ? ham.slice(0, 20000) + "\n…(kesildi)" : ham;
        verso.showStatus(url + " → " + ham.length + " bayt", 4000);
        return "<html><body><pre>" + esc(govde) + "</pre></body></html>";
    });
    verso.registerView("gorunum", "REST Yanıt", function () {
        return "<html><body><p>Komuttan istek yapın: REST İstemcisi → GET İsteği</p></body></html>";
    });
    verso.log("rest istemcisi yüklendi");
})();
