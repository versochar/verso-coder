// Verso Coder mağaza eklentisi: 3 pastel tema
// Temalar galeriye düşer (Ayarlar → Tema).
// @name Pastel Temalar
// @version 1.0.0
// @permission ui
(function () {
    var KOYU = '{"name":"Pastel Koyu","dark":true,"colors":{' +
        '"bg":"#2b2a33","surface":"#35343f","surfaceAlt":"#403f4d","border":"#4e4d5e",' +
        '"text":"#e8e4da","textStrong":"#faf7ef","textDim":"#9a94a8",' +
        '"accent":"#f2a7c3","success":"#b5e48c","warning":"#f0d089","error":"#f08a8a",' +
        '"selection":"#4e4d5e","lineHighlight":"#35343f",' +
        '"gutterBg":"#2b2a33","gutterText":"#6e6c80","gutterActive":"#e8e4da",' +
        '"indentGuide":"#403f4d","bracket":"#a8d8f0","cursor":"#f2a7c3","scrollbar":"#4e4d5e",' +
        '"syntax":{"keyword":"#a8d8f0","string":"#c3e6a3","comment":"#8a8699",' +
        '"number":"#f0c987","func":"#f2c6a7","type":"#9fd8cb"}}}';
    var ACIK = '{"name":"Pastel Açık","dark":false,"colors":{' +
        '"bg":"#faf6ef","surface":"#f1ece1","surfaceAlt":"#e7dfd0","border":"#d3c9b6",' +
        '"text":"#4a4438","textStrong":"#2e2a22","textDim":"#8a8172",' +
        '"accent":"#d96a8b","success":"#5d9b4a","warning":"#b07a2a","error":"#c04545",' +
        '"selection":"#f0d9e2","lineHighlight":"#f1ece1",' +
        '"gutterBg":"#faf6ef","gutterText":"#b3aa97","gutterActive":"#4a4438",' +
        '"indentGuide":"#e7dfd0","bracket":"#4a90c2","cursor":"#d96a8b","scrollbar":"#d3c9b6",' +
        '"syntax":{"keyword":"#4a90c2","string":"#5d9b4a","comment":"#a39e93",' +
        '"number":"#c27d4a","func":"#b0578f","type":"#3aa68f"}}}';
    var KONTRAST = '{"name":"Pastel Kontrast","dark":true,"colors":{' +
        '"bg":"#101014","surface":"#1b1b22","surfaceAlt":"#26262f","border":"#3d3d4d",' +
        '"text":"#f5f3ff","textStrong":"#ffffff","textDim":"#a8a5c0",' +
        '"accent":"#ffb3c7","success":"#c3f09a","warning":"#ffd166","error":"#ff8a8a",' +
        '"selection":"#3d3d4d","lineHighlight":"#1b1b22",' +
        '"gutterBg":"#101014","gutterText":"#5e5c78","gutterActive":"#f5f3ff",' +
        '"indentGuide":"#26262f","bracket":"#9adcff","cursor":"#ffb3c7","scrollbar":"#3d3d4d",' +
        '"syntax":{"keyword":"#9adcff","string":"#c3f09a","comment":"#7e7c99",' +
        '"number":"#ffd166","func":"#ffc7d9","type":"#8ef0d2"}}}';
    verso.registerTheme("Pastel Koyu", KOYU);
    verso.registerTheme("Pastel Açık", ACIK);
    verso.registerTheme("Pastel Kontrast", KONTRAST);
    verso.log("pastel temalar yüklendi (3 tema)");
})();
