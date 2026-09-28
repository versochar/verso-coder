// Verso Coder mağaza eklentisi: düzenli ifade deneme tahtası
// Bağımsız değişken biçimi: "kalıp\nmetin" (kalıp //.../ bayraksız yazılır).
// @name Regex Deneme
// @version 1.0.0
// @permission ui
(function () {
    function dene(kalip, metin) {
        var re;
        try {
            re = new RegExp(kalip, "g");
        } catch (e) {
            return "hata: " + e.message;
        }
        var out = [], m, n = 0;
        while ((m = re.exec(metin)) !== null) {
            n++;
            var g = [], i;
            for (i = 1; i < m.length; i++) g.push(m[i]);
            out.push(n + ". [" + m.index + "-" + (m.index + m[0].length) + "] '" +
                     m[0] + "'" + (g.length ? " (" + g.join(", ") + ")" : ""));
            if (n > 50) { out.push("…(50'de kesildi)"); break; }
            if (m[0] === "") re.lastIndex++;
        }
        return n ? out.join("\n") : "eşleşme yok";
    }
    verso.registerCommand("dene", "Regex Dene", function (arg) {
        var kalip = arg, metin = "";
        if (arg && arg.indexOf("\n") >= 0) {
            kalip = arg.slice(0, arg.indexOf("\n"));
            metin = arg.slice(arg.indexOf("\n") + 1);
        } else {
            kalip = verso.inputBox("Kalıp:", "");
            if (!kalip) return "vazgeçildi";
            metin = verso.inputBox("Metin:", "");
        }
        var s = dene(kalip, metin);
        verso.showStatus(s.split("\n")[0], 5000);
        return s;
    });
    verso.log("regex deneme yüklendi");
})();
