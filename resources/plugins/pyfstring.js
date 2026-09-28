// Verso Coder mağaza eklentisi: % ve .format() -> f-string
// Yalnız konumsal (sıralı) yer tutucular çevrilir.
// @name Python f-string
// @version 1.0.0
// @permission fs.read
// @permission fs.write
// @permission ui
(function () {
    function yuzde(line) {
        var m = line.match(/("([^"\\]|\\.)*"|'([^'\\]|\\.)*')\s*%\s*(.+)$/);
        if (!m) return { line: line, n: 0 };
        var s = m[1], quote = s.charAt(0), govde = s.slice(1, -1), kalan = m[4];
        var args;
        if (/^\s*\(/.test(kalan)) {
            var ic = kalan.replace(/^\s*\(/, "").replace(/\)\s*;?\s*$/, "");
            args = ic.split(",");
        } else {
            args = [kalan.replace(/;\s*$/, "")];
        }
        var ai = 0, cikti = "";
        for (var i = 0; i < govde.length; i++) {
            if (govde.charAt(i) === "%" && i + 1 < govde.length &&
                /[sdfxoceg]/.test(govde.charAt(i + 1))) {
                cikti += "{" + (args[ai++] || "").replace(/^\s+|\s+$/g, "") + "}";
                i++;
            } else if (govde.charAt(i) === "%" && govde.charAt(i + 1) === "%") {
                cikti += "%"; i++;
            } else {
                cikti += govde.charAt(i);
            }
        }
        if (ai === 0) return { line: line, n: 0 };
        var bas = line.slice(0, line.indexOf(s));
        return { line: bas + "f" + quote + cikti + quote, n: 1 };
    }
    function noktaFormat(line) {
        var m = line.match(/("([^"\\]|\\.)*"|'([^'\\]|\\.)*')\.format\s*\((.*)\)\s*;?\s*$/);
        if (!m) return { line: line, n: 0 };
        if (/=\s*/.test(m[4]) && /[A-Za-z_][A-Za-z0-9_]*\s*=/.test(m[4]))
            return { line: line, n: 0 }; // adlandırmalı: dokunma
        var s = m[1], quote = s.charAt(0), govde = s.slice(1, -1);
        var args = m[4].split(","), ai = 0, cikti = "";
        for (var i = 0; i < govde.length; i++) {
            if (govde.charAt(i) === "{" && govde.charAt(i + 1) === "}") {
                cikti += "{" + (args[ai++] || "").replace(/^\s+|\s+$/g, "") + "}";
                i++;
            } else {
                cikti += govde.charAt(i);
            }
        }
        if (ai === 0) return { line: line, n: 0 };
        var bas = line.slice(0, line.indexOf(s));
        return { line: bas + "f" + quote + cikti + quote, n: 1 };
    }
    verso.registerCommand("cevir", "f-string'e Çevir", function (arg) {
        var p = "", t = arg;
        if (!t) {
            p = verso.currentFile();
            if (!p) { verso.showStatus("Önce bir dosya açın", 3000); return "dosya yok"; }
            t = verso.readFile(p);
        }
        var lines = (t || "").split("\n"), n = 0, i, r;
        for (i = 0; i < lines.length; i++) {
            if (lines[i].indexOf("f\"") >= 0 || lines[i].indexOf("f'") >= 0) continue;
            r = yuzde(lines[i]);
            if (!r.n) r = noktaFormat(lines[i]);
            if (r.n) { lines[i] = r.line; n++; }
        }
        if (!arg) {
            verso.writeFile(p, lines.join("\n"));
            verso.showStatus(n + " satır çevrildi", 3000);
        }
        return n + " satır";
    });
    verso.log("python f-string yüklendi");
})();
