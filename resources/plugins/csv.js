// Verso Coder mağaza eklentisi: CSV tablo görünümü
// @name CSV Tablo
// @version 1.0.0
// @permission fs.read
// @permission ui
(function () {
    function esc(s) {
        return s.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;");
    }
    function satir(l) {
        var out = [], cur = "", tirnak = false, i, c;
        for (i = 0; i < l.length; i++) {
            c = l[i];
            if (c === '"') {
                if (tirnak && l[i + 1] === '"') { cur += '"'; i++; } else { tirnak = !tirnak; }
            } else if (c === "," && !tirnak) {
                out.push(cur); cur = "";
            } else {
                cur += c;
            }
        }
        out.push(cur);
        return out;
    }
    function tablo(text) {
        var lines = text.split("\n"), html = ["<html><body><table border=\"1\">"],
            ilk = true, i, h, j, etiket;
        for (i = 0; i < lines.length; i++) {
            if (/^\s*$/.test(lines[i])) continue;
            h = satir(lines[i]);
            etiket = ilk ? "th" : "td";
            html.push("<tr>");
            for (j = 0; j < h.length; j++)
                html.push("<" + etiket + ">" + esc(h[j]) + "</" + etiket + ">");
            html.push("</tr>");
            ilk = false;
        }
        html.push("</table></body></html>");
        return html.join("\n");
    }
    function kaynak(arg) {
        if (arg) return arg;
        var p = verso.currentFile();
        if (!p) { verso.showStatus("Önce bir dosya açın", 3000); return ""; }
        return verso.readFile(p);
    }
    verso.registerCommand("tablo", "CSV Tablo", function (arg) {
        var t = kaynak(arg);
        if (!t) return "boş";
        return tablo(t);
    });
    verso.registerView("gorunum", "CSV", function () {
        return tablo(kaynak(""));
    });
    verso.log("csv tablo yüklendi");
})();
