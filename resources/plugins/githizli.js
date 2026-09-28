// Verso Coder mağaza eklentisi: sık git komutlarını terminale gönderir
// @name Git Hızlı Komut
// @version 1.0.0
// @permission ui
(function () {
    var KOMUT = [
        ["Durum", "git status --short"],
        ["Son 10 commit", "git log --oneline -10"],
        ["Fark özeti", "git diff --stat"],
        ["Dallar", "git branch"]
    ];
    function bul(ad) {
        for (var i = 0; i < KOMUT.length; i++)
            if (KOMUT[i][0] === ad || KOMUT[i][1] === ad) return KOMUT[i][1];
        return "";
    }
    verso.registerCommand("hizli", "Git Hızlı Komut", function (arg) {
        var cmd = bul(arg || "");
        if (!cmd) {
            var ad = ["Durum", "Son 10 commit", "Fark özeti", "Dallar"];
            var sec = verso.quickPick(ad, "Git:");
            if (!sec) return "vazgeçildi";
            cmd = bul(sec);
        }
        verso.sendTerminal(cmd + "\n");
        verso.showStatus("Terminale gönderildi: " + cmd, 3000);
        return "gönderildi: " + cmd;
    });
    verso.log("git hızlı komut yüklendi");
})();
