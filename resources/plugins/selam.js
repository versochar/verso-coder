// Verso Coder örnek eklentisi: durum + giriş (Stage 29 v2 API)
// @name Selam Durumu
// @version 1.0.0
// @permission ui
(function () {
    verso.log("selam eklentisi yüklendi");
    var count = verso.getGlobalState("count") || "0";
    verso.registerCommand("selam", "Selam Ver", function () {
        var ad = verso.inputBox("Adınız?", "Dünya");
        if (!ad) return "vazgeçildi";
        count = String(Number(count) + 1);
        verso.setGlobalState("count", count);
        verso.showStatus("Selam " + ad + " (#" + count + ")", 4000);
        return "selam:" + ad;
    });
})();
