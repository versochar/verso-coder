// Verso Coder mağaza eklentisi: 3 günlük hava durumu (ücretsiz API)
// @name Hava Durumu
// @version 1.0.0
// @permission net
// @permission ui
(function () {
    var SEHIR = {
        "İstanbul": "41.0,29.0", "Ankara": "39.9,32.9", "İzmir": "38.4,27.1",
        "Bursa": "40.2,29.1", "Londra": "51.5,-0.1"
    };
    function kod(c) {
        if (c === 0) return "Açık";
        if (c <= 3) return "Parçalı";
        if (c <= 48) return "Sisli";
        if (c <= 57) return "Çisenti";
        if (c <= 67) return "Yağmurlu";
        if (c <= 77) return "Karlı";
        if (c <= 82) return "Sağanak";
        return "Fırtına";
    }
    verso.registerCommand("durum", "Hava Durumu", function (arg) {
        var k = arg, ad = "";
        if (SEHIR[k]) { ad = k; k = SEHIR[k]; }
        if (!k || !/^-?\d+(\.\d+)?,-?\d+(\.\d+)?$/.test(k)) {
            var sec = verso.quickPick(
                ["İstanbul", "Ankara", "İzmir", "Bursa", "Londra"], "Şehir:");
            if (!sec) return "vazgeçildi";
            ad = sec; k = SEHIR[sec];
        }
        var parca = k.split(",");
        var url = "https://api.open-meteo.com/v1/forecast?latitude=" + parca[0] +
            "&longitude=" + parca[1] +
            "&daily=weathercode,temperature_2m_max,temperature_2m_min" +
            "&timezone=auto&forecast_days=3";
        var ham = verso.fetch(url, 12000);
        if (!ham) return "yanıt yok";
        try {
            var o = JSON.parse(ham).daily, satir = [], i;
            for (i = 0; i < o.time.length; i++) {
                satir.push(o.time[i] + ": " + kod(o.weathercode[i]) + " " +
                           o.temperature_2m_max[i] + "°/" + o.temperature_2m_min[i] + "°");
            }
            var sonuc = (ad ? ad + " — " : "") + satir.join(" · ");
            verso.showStatus(sonuc, 8000);
            return sonuc;
        } catch (e) {
            return "çözümlenemedi";
        }
    });
    verso.log("hava durumu yüklendi");
})();
