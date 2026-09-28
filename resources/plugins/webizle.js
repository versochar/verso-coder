// Verso Coder mağaza eklentisi: web önizleme (görünüm)
// Açık HTML'in göreli <link>/<script src> içeriklerini okuyup içine gömer.
// @name Web Önizleme
// @version 1.0.0
// @permission fs.read
// @permission ui
(function () {
    function esc(s) {
        return s.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;");
    }
    function klasor(path) {
        var s = (path || "").replace(/\\/g, "/");
        var k = s.lastIndexOf("/");
        return k >= 0 ? s.slice(0, k + 1) : "";
    }
    function gomy(text, kok) {
        var cikti = text;
        cikti = cikti.replace(
            /<link([^>]*?)rel\s*=\s*["']stylesheet["']([^>]*?)href\s*=\s*["']([^"']+)["']([^>]*?)>/gi,
            function (m, a, b, href) {
                if (/^(https?:|data:|\/)/.test(href)) return m;
                var css = "";
                try { css = verso.readFile(kok + href); } catch (e) { return m; }
                if (!css) return m;
                return "<style>\n" + css + "\n</style>";
            });
        cikti = cikti.replace(
            /<script([^>]*?)src\s*=\s*["']([^"']+)["']([^>]*?)>\s*<\/script\s*>/gi,
            function (m, a, src) {
                if (/^(https?:|data:|\/)/.test(src)) return m;
                var js = "";
                try { js = verso.readFile(kok + src); } catch (e) { return m; }
                if (!js) return m;
                return "<script>\n" + js + "\n</script>";
            });
        return cikti;
    }
    function kaynak(arg) {
        if (arg || arg === "") return { kok: "", text: arg };
        var p = verso.currentFile();
        if (!p) { verso.showStatus("Önce bir dosya açın", 3000); return null; }
        return { kok: klasor(p), text: verso.readFile(p) };
    }
    verso.registerCommand("goster", "Web Önizleme", function (arg) {
        var k = (arg || arg === "") ? { kok: "", text: arg } : kaynak();
        if (!k || !k.text) return "boş";
        return gomy(k.text, k.kok);
    });
    verso.registerView("gorunum", "Web", function () {
        var k = kaynak();
        if (!k || !k.text) return "<html><body><p>Önce bir HTML dosyası açın</p></body></html>";
        return gomy(k.text, k.kok);
    });
    verso.log("web önizleme yüklendi");
})();
