// Verso Coder mağaza eklentisi: Lorem Ipsum üretici
// Sayı verilirse üretip döner; verilmezse sorup açık dosyanın sonuna ekler.
// @name Lorem Üretici
// @version 1.0.0
// @permission fs.read
// @permission fs.write
// @permission ui
(function () {
    var CUMLE = [
        "Lorem ipsum dolor sit amet, consectetur adipiscing elit.",
        "Sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.",
        "Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris.",
        "Duis aute irure dolor in reprehenderit in voluptate velit esse.",
        "Excepteur sint occaecat cupidatat non proident, sunt in culpa.",
        "Curabitur pretium tincidunt lacus, nec iaculis est fermentum vel.",
        "Nulla facilisi cras fermentum odio eu feugiat pretium nibh.",
        "Vestibulum mattis ullamcorper velit sed ullamcorper morbi tincidunt."
    ];
    function uret(n) {
        var p = [], i, j, s;
        for (i = 0; i < n; i++) {
            s = [];
            for (j = 0; j < 4; j++) s.push(CUMLE[(i * 4 + j) % CUMLE.length]);
            p.push(s.join(" "));
        }
        return p.join("\n\n");
    }
    verso.registerCommand("uret", "Lorem Ipsum Üret", function (arg) {
        var n = parseInt(arg || verso.inputBox("Paragraf sayısı?", "3"), 10);
        if (!(n > 0)) n = 3;
        if (n > 50) n = 50;
        var t = uret(n);
        if (!arg) {
            var p = verso.currentFile();
            if (!p) { verso.showStatus("Önce bir dosya açın", 3000); return "dosya yok"; }
            var eski = verso.readFile(p) || "";
            verso.writeFile(p, eski + (eski ? "\n\n" : "") + t);
            verso.showStatus(n + " paragraf eklendi", 3000);
        }
        return t;
    });
    verso.log("lorem üretici yüklendi");
})();
