// Verso Coder örnek eklentisi (Stage 22)
// İzin bildirimi: // @permission satırları PluginEngine tarafından okunur.
// @name Echo
// @version 1.0.0
// @permission fs.read
(function () {
    verso.log("echo eklentisi yüklendi");
    verso.registerCommand("selam", "Selam Ver", function () {
        verso.log("Selam! Bu mesaj örnek eklentiden geliyor.");
        return "selam";
    });
})();
