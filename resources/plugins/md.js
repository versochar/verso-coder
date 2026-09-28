// Verso Coder mağaza eklentisi: Markdown önizleme
// Açık .md dosyasını (ya da verilen metni) HTML görünüme çevirir.
// @name Markdown Önizleme
// @version 1.0.0
// @permission fs.read
// @permission ui
(function () {
    function esc(s) {
        return s.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;");
    }
    function bicim(s) {
        s = esc(s);
        s = s.replace(/`([^`]+)`/g, "<code>$1</code>");
        s = s.replace(/\*\*([^*]+)\*\*/g, "<strong>$1</strong>");
        s = s.replace(/\*([^*]+)\*/g, "<em>$1</em>");
        s = s.replace(/\[([^\]]+)\]\(([^)]+)\)/g, "<a href=\"$2\">$1</a>");
        return s;
    }
    function cevir(text) {
        var satir = text.split("\n"), html = [], listede = false, i, line, m;
        for (i = 0; i < satir.length; i++) {
            line = satir[i];
            if ((m = line.match(/^(#{1,6})\s+(.*)$/))) {
                if (listede) { html.push("</ul>"); listede = false; }
                html.push("<h" + m[1].length + ">" + bicim(m[2]) +
                          "</h" + m[1].length + ">");
            } else if ((m = line.match(/^\s*[-*]\s+(.*)$/))) {
                if (!listede) { html.push("<ul>"); listede = true; }
                html.push("<li>" + bicim(m[1]) + "</li>");
            } else if (/^\s*$/.test(line)) {
                if (listede) { html.push("</ul>"); listede = false; }
            } else {
                if (listede) { html.push("</ul>"); listede = false; }
                html.push("<p>" + bicim(line) + "</p>");
            }
        }
        if (listede) html.push("</ul>");
        return "<html><body>\n" + html.join("\n") + "\n</body></html>";
    }
    function kaynak(arg) {
        if (arg) return arg;
        var p = verso.currentFile();
        if (!p) { verso.showStatus("Önce bir dosya açın", 3000); return ""; }
        return verso.readFile(p);
    }
    verso.registerCommand("goster", "Markdown Önizleme", function (arg) {
        var t = kaynak(arg);
        if (!t) return "boş";
        return cevir(t);
    });
    verso.registerView("gorunum", "Markdown", function () {
        return cevir(kaynak(""));
    });
    verso.log("markdown önizleme yüklendi");
})();
