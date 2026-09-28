// Verso Coder örnek eklentisi: hızlı not kaydetme (Stage 29 v2 API)
// @name Hızlı Not
// @version 1.0.0
// @permission ui, fs.write, fs.read
(function () {
    verso.log("not eklentisi yüklendi");
    verso.registerCommand("not", "Hızlı Not Kaydet", function () {
        var secenek = verso.quickPick(["Fikir", "Hata", "Yapılacak"], "Tür seç");
        if (!secenek) return "vazgeçildi";
        var metin = verso.inputBox(secenek + ":", "");
        if (!metin) return "vazgeçildi";
        var yol = verso.getWorkspaceState("notYolu") || "/tmp/verso-notlar.md";
        var eski = "";
        try { eski = verso.readFile(yol); } catch (e) { eski = ""; }
        var satir = "- [" + secenek + "] " + metin + "\n";
        // fs.read izni yoksa eski içerik boş kalır, dosya yine de yazılır
        if (verso.writeFile(yol, eski + satir)) {
            verso.showStatus("Not kaydedildi", 3000);
            return "ok";
        }
        return "yazılamadı";
    });
    verso.registerView("notlar", "Notlarım", function () {
        return "<h3>Notlar</h3><p>Hızlı Not ile kaydedilenler <code>/tmp/verso-notlar.md</code> dosyasındadır.</p>";
    });
})();
