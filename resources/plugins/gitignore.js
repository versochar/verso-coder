// Verso Coder mağaza eklentisi: .gitignore üretici
// Şablonlar github/gitignore deposundan çekilir, köke yazılır.
// @name Gitignore Üretici
// @version 1.0.0
// @permission net
// @permission fs.write
// @permission ui
(function () {
    var SABLON = {
        "Python": "Python.gitignore", "Node": "Node.gitignore",
        "Rust": "Rust.gitignore", "C++": "C++.gitignore", "C": "C.gitignore",
        "Java": "Java.gitignore", "Go": "Go.gitignore", "Qt": "Qt.gitignore"
    };
    verso.registerCommand("uret", "Gitignore Üret", function (arg) {
        var dil = arg;
        if (!SABLON[dil]) {
            var ad = ["Python", "Node", "Rust", "C++", "C", "Java", "Go", "Qt"];
            dil = verso.quickPick(ad, "Dil:");
            if (!dil) return "vazgeçildi";
        }
        var url = "https://raw.githubusercontent.com/github/gitignore/main/" +
            SABLON[dil];
        var ham = verso.fetch(url, 15000);
        if (!ham) return "şablon inemedi";
        verso.writeFile(".gitignore",
                        "# " + dil + " (" + "github/gitignore" + ")\n" + ham);
        verso.showStatus(".gitignore yazıldı (" + dil + ")", 4000);
        return ham.split("\n").length + " satır";
    });
    verso.log("gitignore üretici yüklendi");
})();
