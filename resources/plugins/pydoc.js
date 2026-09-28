// Verso Coder mağaza eklentisi: Python docstring yardımcısı
// Docstring'siz def/class'ları Problems'e yazar, Google-stili iskelet ekler.
// @name Python Docstring
// @version 1.0.0
// @permission fs.read
// @permission fs.write
// @permission ui
(function () {
    function bul(text) {
        var lines = text.split("\n"), out = [], i, m, j, s;
        for (i = 0; i < lines.length; i++) {
            m = lines[i].match(/^\s*(def|class)\s+(\w+)\s*(\(([^)]*)\))?/);
            if (!m) continue;
            var govde = false;
            for (j = i + 1; j < lines.length && j < i + 4; j++) {
                s = lines[j].replace(/^\s+/, "");
                if (s === "") continue;
                if (/^("""|''')/.test(s)) { govde = true; break; }
                break;
            }
            if (!govde)
                out.push({ line: i + 1, tur: m[1], ad: m[2], args: m[4] || "" });
        }
        return out;
    }
    function iskelet(b, girinti) {
        var s = [girinti + '"""' + b.ad + ".", ""];
        var args = [];
        var parca = (b.args || "").split(",");
        for (var i = 0; i < parca.length; i++) {
            var a = parca[i].replace(/^\s+|\s+$/g, "").split("=")[0]
                .replace(/^\s+|\s+$/g, "");
            if (a && a !== "self" && a !== "cls") args.push(a);
        }
        if (args.length) {
            s.push("    Args:");
            for (var j = 0; j < args.length; j++) s.push("        " + args[j] + ": ");
            s.push("");
        }
        if (b.tur === "def") s.push("    Returns:");
        s.push('    """');
        return s;
    }
    function kaynak(arg) {
        if (arg) return { path: "", text: arg };
        var p = verso.currentFile();
        if (!p) { verso.showStatus("Önce bir dosya açın", 3000); return null; }
        return { path: p, text: verso.readFile(p) };
    }
    verso.registerCommand("tara", "Docstring Tara", function (arg) {
        var k = kaynak(arg);
        if (!k) return "dosya yok";
        var b = bul(k.text);
        if (k.path) {
            var prob = [], i;
            for (i = 0; i < b.length; i++)
                prob.push({ file: k.path, line: b[i].line,
                            message: "docstring yok: " + b[i].tur + " " + b[i].ad });
            verso.reportProblems(JSON.stringify(prob));
        }
        var s = b.length + " docstring'siz";
        verso.showStatus(s, 4000);
        return s;
    });
    verso.registerCommand("ekle", "Docstring Ekle", function (arg) {
        var k = kaynak(arg);
        if (!k) return "dosya yok";
        var lines = k.text.split("\n"), b = bul(k.text), n = 0, i, g;
        for (i = b.length - 1; i >= 0; i--) {
            g = lines[b[i].line - 1].match(/^(\s*)/)[1] + "    ";
            var sk = iskelet(b[i], g);
            for (var j = sk.length - 1; j >= 0; j--)
                lines.splice(b[i].line, 0, sk[j]);
            n++;
        }
        if (!arg) {
            verso.writeFile(k.path, lines.join("\n"));
            verso.showStatus(n + " docstring eklendi", 3000);
        }
        return n + " eklendi";
    });
    verso.log("python docstring yüklendi");
})();
