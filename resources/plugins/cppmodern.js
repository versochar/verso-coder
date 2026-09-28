// Verso Coder mağaza eklentisi: C++ modernleştirme
// NULL -> nullptr, tek satırlık typedef -> using. Kaba ama güvenli aralık.
// @name C++ Modernleştirici
// @version 1.0.0
// @permission fs.read
// @permission fs.write
// @permission ui
(function () {
    verso.registerCommand("modernlestir", "Modernleştir (C++11)", function (arg) {
        var p = "", t = arg;
        if (!t && t !== "") {
            p = verso.currentFile();
            if (!p) { verso.showStatus("Önce bir dosya açın", 3000); return "dosya yok"; }
            t = verso.readFile(p);
        }
        t = t || "";
        var nul = 0, typ = 0;
        t = t.replace(/\bNULL\b/g, function () { nul++; return "nullptr"; });
        var lines = t.split("\n"), i, m;
        for (i = 0; i < lines.length; i++) {
            m = lines[i].match(/^(\s*)typedef\s+(.+?)\s+([A-Za-z_]\w*)\s*;\s*$/);
            if (m && m[2].indexOf("(") < 0 && m[2].indexOf("[") < 0) {
                lines[i] = m[1] + "using " + m[3] + " = " + m[2] + ";";
                typ++;
            }
        }
        if (!arg && arg !== "") {
            verso.writeFile(p, lines.join("\n"));
            verso.showStatus(nul + " nullptr, " + typ + " using", 3000);
        }
        return nul + " nullptr · " + typ + " using";
    });
    verso.log("c++ modernleştirici yüklendi");
})();
