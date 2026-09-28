// Verso Coder mağaza eklentisi: JSON -> model sınıfı
// Bağımsız değişken biçimi: "dil:json" (dil: cpp | py | ts).
// @name Model Üretici
// @version 1.0.0
// @permission fs.read
// @permission fs.write
// @permission ui
(function () {
    function tur(v) {
        if (v === null || v === undefined) return "?";
        if (typeof v === "boolean") return "bool";
        if (typeof v === "number") return (Math.floor(v) === v) ? "int" : "float";
        if (typeof v === "string") return "str";
        if (Object.prototype.toString.call(v) === "[object Array]") return "list";
        return "obj";
    }
    function buyuk(s) { return s.charAt(0).toUpperCase() + s.slice(1); }
    function cppAlan(ad, v, ek) {
        var t = tur(v), adC = buyuk(ad.replace(/[^A-Za-z0-9_]/g, "_"));
        if (t === "obj") { ek.push(cppSinif(adC, v)); return "    " + adC + " " + ad + ";"; }
        if (t === "list")
            return "    std::vector<std::string> " + ad + ";";
        var m = { str: "std::string", int: "int", float: "double", bool: "bool" };
        return "    " + (m[t] || "std::string") + " " + ad + ";";
    }
    function cppSinif(ad, o) {
        var s = ["struct " + ad + " {"], ek = [], k;
        for (k in o) s.push(cppAlan(k, o[k], ek));
        s.push("};");
        return ek.join("\n") + (ek.length ? "\n" : "") + s.join("\n");
    }
    function pyAlan(ad, v, ek) {
        var t = tur(v), adC = buyuk(ad.replace(/[^A-Za-z0-9_]/g, "_"));
        if (t === "obj") { ek.push(pySinif(adC, v)); return "    " + ad + ": " + adC; }
        var m = { str: "str", int: "int", float: "float", bool: "bool",
                  list: "list", "?": "Any" };
        return "    " + ad + ": " + (m[t] || "Any");
    }
    function pySinif(ad, o) {
        var s = ["@dataclass", "class " + ad + ":"], ek = [], k, govde = false;
        for (k in o) { s.push(pyAlan(k, o[k], ek)); govde = true; }
        if (!govde) s.push("    pass");
        return ek.join("\n") + (ek.length ? "\n" : "") + s.join("\n");
    }
    function tsAlan(ad, v, ek) {
        var t = tur(v), adC = buyuk(ad.replace(/[^A-Za-z0-9_]/g, "_"));
        if (t === "obj") { ek.push(tsArayuz(adC, v)); return "  " + ad + ": " + adC + ";"; }
        var m = { str: "string", int: "number", float: "number", bool: "boolean",
                  list: "unknown[]", "?": "unknown" };
        return "  " + ad + ": " + (m[t] || "unknown") + ";";
    }
    function tsArayuz(ad, o) {
        var s = ["interface " + ad + " {"], ek = [], k;
        for (k in o) s.push(tsAlan(k, o[k], ek));
        s.push("}");
        return ek.join("\n") + (ek.length ? "\n" : "") + s.join("\n");
    }
    verso.registerCommand("uret", "Model Üret (JSON)", function (arg) {
        var dil = "cpp", ham = arg;
        if (!ham) {
            var p = verso.currentFile();
            if (!p) { verso.showStatus("Önce bir dosya açın", 3000); return "dosya yok"; }
            ham = verso.readFile(p);
            var sec = verso.quickPick(["cpp", "py", "ts"], "Dil:");
            if (!sec) return "vazgeçildi";
            dil = sec;
        } else {
            var kes = ham.indexOf(":");
            if (kes > 0 && kes < 5) { dil = ham.slice(0, kes); ham = ham.slice(kes + 1); }
        }
        var o;
        try { o = JSON.parse(ham); } catch (e) { return "hata: " + e.message; }
        if (typeof o !== "object" || o === null) return "hata: kök nesne olmalı";
        var kod;
        if (dil === "py") kod = "from dataclasses import dataclass\nfrom typing import Any\n\n" + pySinif("Model", o);
        else if (dil === "ts") kod = tsArayuz("Model", o);
        else kod = "#include <string>\n#include <vector>\n\n" + cppSinif("Model", o);
        if (!arg) {
            verso.writeFile(verso.currentFile(), kod);
            verso.showStatus("Model üretildi (" + dil + ")", 3000);
        }
        return kod;
    });
    verso.log("model üretici yüklendi");
})();
