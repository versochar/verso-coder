# Verso Coder

VS Code tarzı, sade ve modern C++ (Qt6) kod editörü.

## Özellikler
- **Sol ActivityBar:** Dosya Gezgini (🗂) · Git (⑂) · AI (✦)
- **Sol alt bağımsız ⚙ düğmesi:** dil (TR/EN), tema (dark/light), AI bağlantısı
- **Sekmeli editör:** satır numarası, aktif satır vurgusu, C++/Python/JS renklendirme
- **Git paneli:** branch, status, stage-all, commit, push, pull (`git` CLI üzerinden)
- **AI paneli (Ollama):** model listesi (`/api/tags`), sohbet (`/api/chat`), açık dosyayı bağlama ekleme

## AI / Ollama ayarları (⚙ → AI Bağlantısı)
| Ayar | Ollama karşılığı | Açıklama |
|---|---|---|
| Ollama Adresi | host | örn. `http://localhost:11434` |
| Model | model | örn. `llama3.1`, `codellama`, `qwen2.5-coder` |
| Context Window | `num_ctx` | 512–131072 |
| GPU Backend | bilgi + `num_gpu` anahtarı | **CUDA** (NVIDIA) · **ROCm** (AMD/Linux) · **Vulkan** (genel) · **CPU** (`num_gpu=0`) |
| GPU Offload | `num_gpu` | 0=CPU, 999=tümü |
| Temperature | `temperature` | 0.00–2.00 |
| CPU Threads | `num_thread` | 1–128 |

> Not: Ollama hangi GPU kütüphanesini (CUDA/ROCm/Vulkan) kullanacağını otomatik seçer.
> Backend seçimi burada saklanır; `CPU` seçilirse `num_gpu=0` gönderilir.
> Doğru derlemeyi kullandığını `ollama --version` ve `ollama serve` loglarından doğrula.

## Kurulum

```bash
# Qt6 + derleyici (Arch/Manjaro örneği / Debian benzeri)
sudo pacman -S qt6-base qt6-tools cmake gcc   # Arch
# sudo apt install qt6-base-dev qt6-tools-dev cmake g++   # Debian/Ubuntu

# Ollama (AI için)
curl -fsSL https://ollama.com/install.sh | sh
ollama serve &
ollama pull llama3.1

# Derle
cmake -S . -B build -DCMAKE_PREFIX_PATH=/usr/lib/cmake
cmake --build build -j
./build/verso-coder
```

## Yol haritası (20 + 407 özellik, 36 stage — tamamlandı)

- **Stage 1 — Editör temeli ✅ (yapıldı):** oturum geri yükleme (açık dosyalar + aktif sekme + imleç), otomatik kaydetme (2 sn) + crash yedekleme, harici değişiklik uyarısı, hızlı açma (Ctrl+P fuzzy), font boyutu + sekme genişliği ayarı, status bar'da Ln/Col + LF/CRLF + UTF-8, otomatik parantez kapatma.
- **Stage 5 — Editör gücü ✅ (yapıldı):** çoklu imleç (Alt+tık, Ctrl+D, Esc), kod katlama (gutter ▸/▾, Ctrl+Shift+[/], oturumda kalıcı), akıllı girinti + `}` geri-girinti + blok Tab, bracket eşleşme (derinlik renkli) + girinti kılavuzları, aramada ±3 satır önizleme, split editör (2 grup + taşı/böl + grup oturumu), satır işlemleri (taşı/çoğalt/sırala/trim), .editorconfig, büyük-dosya önizleme modu, hunspell yazım denetimi (TR+EN, F7, sağ-tık öneri).
- **Stage 2 — Navigasyon & Git ✅ (yapıldı):** global arama/değiştir paneli (düz metin/regex, Aa, `*.cpp` filtresi, önizlemeli ağaç, çift tıkla satıra git, toplu değiştir), breadcrumb (kök › klasör › dosya + sınıf/fonksiyon sembol kutusu), hafif minimap (satır çubukları + viewport + tıkla-kaydır, Görünüm'den aç/kapa), Git'te dosya bazlı Stage/Unstage + renkli Diff/Diff(staged) + branch listesi/oluştur/checkout + log grafiği, Ctrl+Shift+F kısayolu.
- **Stage 3 — AI yükseltme ✅ (yapıldı):** streaming yanıt (token token canlı, ■ ile durdurma), bağlam modları (açık dosya / seçili kod / proje-RAG / bağlamsız), hazır komutlar (Açıkla, Düzelt, Test yaz, staged diff'ten Commit msg), yerel RAG (chunk+overlap indeks, anahtar kelime skoru, `İndeksle` + otomatik indeks), çoklu model profili (kaydet/sil/uygula, sıcaklık+num_ctx dahil), sohbet geçmişi (kaydet/yükle/temizle), diff onaylı ajan düzenlemesi (`Dosyaya uygula` → son ``` bloğu → eski/yeni onayı → seçim veya dosya, Ctrl+Z ile geri alınabilir).
- **Stage 4 — Platform ✅ (yapıldı):** minimal LSP istemcisi (clangd/pylsp, JSON-RPC stdio; hover F12/Ctrl+K, tanım atlama, dalgalı kırmızı/sarı alt çizgi + ⚠ Sorunlar paneli), komut paleti (Ctrl+Shift+P, fuzzy) + ⚙ Kısayollar sekmesinde 12 eylem için QKeySequenceEdit ile özelleştirme, alt terminal dock (kalıcı bash + Derle & Çalıştır: cpp/c/py/sh/js şablonları, F5), tema içe aktarma (custom.qss), büyük-dosya modu (eşik üstü renklendirme kapalı + örneklemeli minimap + breadcrumb cap), arama ve RAG indeksleme arka planda (Qt Concurrent).
- **Stage 6 — Proje & Git derinliği ✅ (yapıldı):** .gitignore dosya ağacı filtresi (`*`, `?`, `**`, `{}`, `!`, `/` kök çapası, `dizin/`), uzantıya göre renkli dosya ikonları, sürükle-bırak taşıma (açık sekmelerin yolu otomatik güncellenir), hunk bazlı diff (yan yana/birleşik görünüm + hunk stage/discard, `git apply` yaması), Git araç sekmeleri (geçmiş grafiği + cherry-pick/revert, stash push/apply/pop/drop, uzak depo ekleme/fetch/push + ahead/behind), TODO/FIXME/HACK tarama paneli (arka plan, çift tıkla satıra git), klasör karşılaştırma (yalnız/farklı/aynı + çift tıkla dosya diff'i), güvenli silme (çöp kutusu → geri yükle/temizle), proje bazlı oturum + adlandırılmış anlık görüntüler (kaydet/yükle).
- **Stage 7 — AI Ajan 2.0 ✅ (yapıldı):** araç kullanan çok adımlı ajan döngüsü (plan → `<tool_call>` → gözlem → devam, adım limiti), güvenli araç yürütücü (read_file / write_file / list_dir / search / run_command / get_problems; proje kökü sandbox'ı, yazma+komut için onay), çoklu dosya düzenleme kuyruğu (`PatchQueue` + LCS tabanlı unified diff + `PatchReviewDialog` ile toplu onay/uygula/reddet), `@`-bağlam etiketleri (`@dosya`, `@dosya:10-25`, `@seçim`, `@sorunlar`, `@proje[:sorgu]`), çoklu sohbet oturumu (`ChatStore`: oluştur/yeniden adlandır/sil/geçiş + JSON kalıcılık), token/maliyet sayacı (Ollama `prompt_eval_count`/`eval_count` → oturum toplamı + ~$), Sorunlar panelinden "AI ile düzelt", komut paletinden `AI Ajan Modu`.
- **Stage 8 — Platform & Dağıtım ✅ (yapıldı):** komut satırı argümanları (`--line N`, `--new`, `--command ID`, `--help`, `--version`, dosya/klasör) + tek örnek (single instance; ikinci çağrı dosyayı/komutu çalışan pencereye iletir, `--new` ile ayrı pencere), araç zinciri teşhisi (git/cmake/g++/clang/clangd/pylsp/python/ollama/bash tespiti + sürüm, arka planda tarama), performans profili (başlangıç/oturum/CLI işaretleri + RSS bellek, `PerfMonitor`), `tasks.json` görev çalıştırıcı (VS Code benzeri; alt panelde **Tasks** sekmesi, çalıştır/durdur/`tasks.json` oluştur, canlı çıktı), kısayol profilleri (Varsayılan / VS Code / JetBrains / Vim; Kısayollar sekmesinden tek tıkla uygula), ayarları JSON olarak dışa/içe aktarma (`SettingsIO`), proje çalışma alanı ayarları (`.verso/workspace.json` — font/sekme/büyük-dosya/kısayol profili/görev geçersiz kılmaları), zaman damgalı yedekler + kurtarma (`BackupManager`, 30 yedek sınırı, Sistem Teşhisi → Yedekler'den geri yükle/sil), Sistem Teşhisi / Hakkında diyaloğu (`F1`; sürüm + yenilikler + güncelleme manifesti denetimi) ve dağıtım (`CMake install` kuralları, `.desktop`, AppData metainfo, AppImage üretim betiği `packaging/build-appimage.sh`).
- **Stage 9 — Tema Motoru & Modern Görsel Dil ✅ (yapıldı):** token tabanlı tema motoru (`ThemeTokens` JSON paleti → `QssBuilder` ile tüm uygulama QSS'i otomatik üretilir; elle QSS yok), 10 hazır tema (Dark+, Light, Nord, One Dark, Dracula, Monokai, Solarized Dark/Light, GitHub Light, Catppuccin Mocha) + tema galerisi (`Ctrl+K, Ctrl+T`; mini editör önizlemeli kartlar, çift tıkla anında uygula), vurgu rengi seçici (7 hazır + özel; hover/pressed/soft türevleri otomatik hesaplanır), SVG ikon sistemi (16 Feather-tarzı ikon, `IconTheme` ile temaya göre yeniden renklenir; emoji butonlar gitti), modern sekme çubuğu (accent alt çizgili aktif sekme, ● kirli göstergesi, sağ tıkla sekme listesi taşma menüsü), editör görsel ayarları (tema renkli aktif satır/gutter/parantez/diagnostic, sayı renklendirme, imleç genişliği 1-6px, satır yüksekliği 1.0-2.0x), tipografi sistemi (UI + editör fontu ayrı; aile/boyut/harf aralığı/ligature), yerleşim ölçü sistemi (`UiMetrics`: boşluk/yarıçap/ölçek tokenları), zengin durum çubuğu (tıklanabilir çipler: git dalı, sorun sayısı, Ln:Col→satıra git, dil/girinti, LF/CRLF, UTF-8) ve tema dışa/içe aktarma JSON (Galeri → İçe/Dışa Aktar).
- **Stage 10 — Mikro-etkileşim & Cila ✅ (yapıldı):** toast bildirim sistemi (`ToastManager`: sağ altta yığılan, önem rengi kenarlıklı, aynı mesaj tekilleşen, otomatik kaybolan bildirimler — kaydetme, tema değişimi, zoom, profil aktarımı gibi olaylarda), animasyon altyapısı (`Animator`: fade/slide/geometri geçişleri; "azaltılmış hareket" tercihinde hepsi anında uygulanır — Ayarlar → Görünüm), yeniden tasarlanmış karşılama ekranı (`WelcomeView`: hızlı başlangıç kartları, son dosyalar listesi, kısayol ipuçları; tüm sekmeler kapanınca otomatik görünür), boş durum görünümü (`EmptyState`: ikon + başlık + eylem butonu; Sorunlar paneli temizken "Sorun yok" kartı), komut paleti cilası (son kullanılan komutlar ★ ile öne gelir ve kalıcıdır, sonuç sayacı, gelişilmiş yer tutucu), renkli minimap (çubuklar satır içeriğine göre sözdizimi renklerinde: yorum/dize/anahtar kelime) + kelime diff'i (Diff görünümünde değişen satırlarda yalnızca farklı kelimeler vurgulanır, LCS tabanlı), yerleşim ön ayarları + Zen modu (`LayoutPresets`: Standart/Editör/Zen; F11 ile geçiş, kalıcı), erişilebilirlik/ölçek (`Ctrl+=` / `Ctrl+-` / `Ctrl+0` yakınlaştırma; arayüz + editör fontu birlikte ölçeklenir) ve kişiselleştirme profili (`UiProfile`: tüm görünüm tercihleri tek JSON dosyasına dışa/içe aktarılır — komut paletinden).
- **Stage 11 — Editör Görsel Derinliği ✅ (yapıldı):** gökkuşağı parantezler (`BracketDepth`: iç içelik seviyesine göre 6 renk, dize/yorum içi yok sayılır; koyu/açık tema paleti) + aktif girinti kılavuzu (imleç seviyesi vurgulu), zengin gutter (kalın aktif satır numarası, git diff şeritleri: yeşil eklenen/mavi değişen/kırmızı silinen, hover'da satır vurgusu + katlama oku), yapışkan kaydırma (`ScopeChain`: girinti tabanlı kapsam zinciri "class Foo › bar()", tıklayınca kapsama atlar), minimap 2.0 (diff + arama işareti şeritleri, hover'da gerçek metinli büyüteç), editör-içi bulma çubuğu (`Ctrl+F`: tüm eşleşmeler + aktif vurgu + "n/m" sayacı, F3/Shift+F3 gezinme; overview ruler + minimap senkron), zengin hover kartı (`HoverCard`: başlıklı, renkli, ``` kod bloklu temalı kart), boşluk görünümü + sütun cetveli (Ayarlar → Görünüm; CRLF satırlarında ␍ işareti), breadcrumb rafinesi (dosya ikonu, ▣ sınıf / ƒ fonksiyon önekleri, sınıflar önce), ince kaydırma çubukları (8px, hover'da belirginleşir), hedef satır flaşı (satıra git/LSP atlamada accent parlaması) + akıllı seçim genişletme (`Shift+Alt+Right`: kelime → satır → {} bloğu → belge).
- **Stage 12 — Uyarlanabilir & Kişiselleştirilebilir Arayüz ✅ (yapıldı):** yoğunluk modu (`Density`: Kompakt/Rahat/Geniş — UiMetrics ölçeği + UI font farkı; komut paletinden döngüyle değişir), canlı tema editörü (`ThemeEditorDialog`: 26 renk alanı renk seçicilerle, anlık önizleme, doğrulanmış JSON kaydetme) + AI tema üretici (Ollama'ya tarif → şemalı istem → `ThemeValidator` doğrulaması → önizleme), otomatik tema (`AutoTheme`: sistem açık/koyu takibi + 07–19 zamanlanmış gündüz/gece, 60 sn denetim), çoklu görünüm profili (`ProfileStore`: isimli kayıt/yükle/sil + durum çubuğunda ●/○ hızlı değiştirici), durum çubuğu özelleştirme (6 çip aç/kapa, Ayarlar → Görünüm), odak/daktilo modu (imleç satırı ortalanır), erişilebilirlik paketi (`high-contrast` teması + `ColorBlind` deuteranopi/protanopi/tritanopi dönüşümü + odak halkaları), özel pencere başlığı (`TitleBar`: frameless sürükle/çift-tık/büyüt/kapat; ayarlardan kapatılabilir), panel sürükle-bırak düzeni (alt sekme sırası kalıcı + yan sayfa aç/kapa), görsel dışa aktarma (pencere PNG kaydı).
- **Stage 13 — Dil Zekâsı ✅ (yapıldı):** `LspClient` genelleştirmesi (genel `request()`/`notify()`/`cancelRequest`, geniş istemci yetenekleri, sunucu yetenekleri `hasCap()` ile desteklenmeyen özellik otomatik pasif), otomatik tamamlama (`CompletionList` parse/filtre/sırala + `CompletionPopup`: kind ikonu, detay; Ctrl+Space + yazarken debounce), tüm referanslar (`LocationSet`: Location/LocationLink, dosyaya göre grup + `ReferencesDialog`, Shift+F12), yeniden adlandırma (`TextEdits` WorkspaceEdit + `RenamePreviewDialog` önizleme, çok dosyalı toplu uygula, F2), hızlı düzeltme (`CodeActionList` + 💡 menü, edit uygula / `workspace/executeCommand`, Ctrl+.), imza yardımı (`SignatureHelp` + otomatik `(` tetikleme, Ctrl+Shift+Space), belge simgeleri (`SymbolTree` + `SymbolPickerDialog` outline, Ctrl+Shift+O) + çalışma alanı simgeleri (Ctrl+T), biçimlendirme (`TextEdits::apply` + Shift+Alt+F + `editor/formatOnSave`), satır içi ipuçları (`InlayHintList` + `CodeEditor` overlay, ayarlanabilir), semantik renklendirme (`SemanticTokens` delta-decode + legend → tema renkleri katmanı, ayarlanabilir), çağrı hiyerarşisi (`CallHierarchyTree` + `CallHierarchyDialog`: gelen/giden, tembel alt seviye, çift tıkla atla).
- **Stage 14 — Hata Ayıklama & Test ✅ (yapıldı):** GDB/MI2 sürücüsü (`GdbDriver`: tokenli komut kuyruğu, kesme/yürüt/adım/yığın/değişken/değerlendir; `MiParser` saf çözümleyici), gutter'dan kesme noktası (kırmızı nokta, sol 16px katlama oku korunur; F9 + `BreakpointStore` kalıcılık + koşullu/log noktası alanı), çalışan çerçeve oku (sarı satır + ➤, dosyaya atlar), `DebugPanel` (Başlat/Durdur/Devam/AdımÜstü/Adımİçi/Bitir + Yığın/Değişkenler/KesmeNoktaları/Konsol sekmeleri; ifade değerlendirme + ham gdb konsolu), `launch.json` (`.verso/launch.json`: program/args/cwd/preBuild/stopAtEntry; yoksa şablon oluşturup açar), test gezgini (`TestExplorer` + `TestDiscovery`: ctest/gtest/pytest/unittest bulur, listeler, tekli/tümlü çalıştırır, ✓/✗/○ + süre + günlük; çöken test fail sayılır), kapsama (`GcovParser`: `gcov --stdout` → yeşil/kırmızı satır ısı haritası + yüzde), problem eşleştiriciler (`ProblemMatcher`: gcc/clang + python traceback + genel dosya:satır → Sorunlar paneli; test çıktıları otomatik taranır), sunucu hazır-değil kuyruğu (`withLspReady`: 12 LSP komutu gdb/LSP ısınırken bekleyip otomatik çalışır).
- **Stage 15 — AI Hattı ✅ (yapıldı):** hayalet tamamlama (`GhostCompletion` FIM istemi + yanıt temizliği + tetik kuralları; `CodeEditor` soluk italik hayalet, Tab kabul / Esc vazgeç, 800 ms debounce, bayat yanıt eleme; varsayılan kapalı, Ayarlar → AI), satır içi yeniden yazma (`InlineEdit` tarif + seçim → `ApplyEditDialog` onaylı uygula) + tek tuşla düzelt, fonksiyona belge yorumu (`DocGen`: imleç üstü tanım bulma, dile göre yorum öneki), test üretimi (`TestGen`: `tests/test_*.cpp` / `test_*.py` hedef çözümleyici, üzerine yazma onayı, yeni sekmede açma), AI commit mesajı (`CommitMsg` Conventional doğrana + `GitPanel` mesaj kutusunu doldurma), sembol açıklama balonu (temalı `HoverCard`), özel istem kitaplığı (`PromptLibrary` + diyalog: kaydet/çalıştır/sil, AI paneline gönderir), son AI kodunu uygula (komut paletinden), Ollama model indirme (`OllamaClient::pull`: akışlı ilerleme + durum çubuğu yüzdesi), bağlam bütçesi (`ContextBudget`: kaba token tahmini + dengeli kırpma, tüm AI akışlarında otomatik).
- **Stage 16 — Uzaktan Geliştirme ✅ (yapıldı):** SSH oturumu (`SshSession`: OpenSSH CLI tabanlı, port/anahtar/jump-host keep-alive, BatchMode + katı host-key varsayılanı ve profil başına TOFU (`trustNewHosts`); CMake'te libssh2 varsa `VERSO_HAVE_LIBSSH2` ile native yola hazır), bağlantı profilleri (`ConnectionProfile` + `RemoteConnectDialog`: kaydet/seç/sil, parola kalıcı yazılmaz), uzak dosya gezgini (`RemoteExplorer`: `command ls` ile kabuk takma adlarına dayanıklı listeleme, aç/yeni/sil/yükle/indir), uzak dosyayı editörde aç/kaydet (`ssh://` belgeler + mtime çakışma sorusu, toplu kayıtta sessiz atlama), uzak terminal (`RemoteTerminal`: pty `-t`, TOFU/parolaya izinli etkileşimli kabuk, `\x03` → Ctrl+C), uzak LSP (`LspTransport` + `LspClient::startRemote`: `ssh hedef -- clangd` stdio köprüsü, tanılar `ssh://` URI ile Sorunlar panelinde), uzakta derle (`RemoteTaskRunner`: gcc/clang çıktısı → uzak kök eşlemeli sorunlar), uzak git (`GitRunner`: yerel/uzak aynı arayüz, `git -C kök`), uzak hata ayıklama (`GdbDriver::targetRemote/launchRemote` + SSH tüneli üzerinden `gdbserver`, sembol dosyası yerel aynadan), port yönlendirme (`PortForwarder` + `PortForwardPanel`: `ssh -N -L`, aktif tünel tablosu).
- **Stage 17 — Editör Deneyimi ✅ (yapıldı):** snippet motoru (`SnippetEngine`: `$1/${2:varsayılan}/$0` + ayna + `$$` kaçışı + `$TM_FILENAME/$TM_SELECTED_TEXT/$CLIPBOARD`; `SnippetManager`: dile göre gömülü set + özel set, önek puanlı eşleşme; `SnippetDialog` palet; Tab/Shift-Tab durak gezme, düzenledikçe kayan duraklar, Esc çıkış), LSP katlama aralıkları (`FoldingRanges`: normalize/kırp/birleştir + girinti algısıyla `merge`, `CodeEditor` sunucu bitişlerini saklar), dikey blok seçim (`BlockSelect`: sekme-bilinçli görsel sütun; Alt+sürükle dikdörtgen, Alt+Shift+Yukarı/Aşağı sütun-korumalı imleç), otomatik çift kapatma (`AutoPairs`: kelime-sonrası/dize/yorum korumalı, kapanıştan atlama, seçim sarma, kapanış-öncesi akıllı Enter; `editor/autoClose` ayarı), özel yapıştır (`PasteTransform`: birleştir/virgüllü/tekilleştir/sırala/büyük-küçük/tırnakla/kırp menüsü), outline paneli (`OutlinePanel`: LSP documentSymbol ağacı + `OutlineFallback` regex taraması, süzme, tıkla-git, yazarken debounce tazeleme), yerel geçmiş (`LocalHistory`: kaydetmede zaman damgalı anlık görüntü, mtime-sıralı liste, budama, günü-kurtaran geri yükleme + `TimelinePanel`: önizleme/fark/geri yükle/temizle), değiştirme derinleştirme (`ReplaceEngine`: dahil+hariç glob, dosya-başı önizlemeli `Dosyada` uygulama, durum koruma, regex `$1` uyumluluğu; `SearchPanel` hariç kutusu), minimap tanı çizgileri (hata/uyarı sağ şeridi), git hunk menüsü (imleç satırında fark göster/hunk geri al/hunk stage'le — `DiffEngine` yama üretimi, geri almadan önce otomatik anlık görüntü), kelime tamamlaması (`WordComplete`: açık belgelerden sıklıklı adaylar, `CompletionPopup` ile sunum).
- **Stage 18 — Canlı İşbirliği & Görev Zinciri ✅ (yapıldı):** görev zinciri (`TaskChain`: `dependsOn` topolojik sıra + döngü hatası, `${input:ad}/${workspaceFolder}/${file}` genişletme, `inputs` varsayılanları, `$gcc` problemMatcher; `TaskRunner` ile `tasks.json` uyumlu), eklenti API v1 (`PluginEngine` + `VersoApi`: `QJSEngine` betikleri, `// @permission fs.read|fs.write` bildirimi, izinsiz erişimde `permissionDenied`, `registerCommand` + `callCommand`), canlı işbirliği (`CollabMerge`: hello/cursor/edit/sync/bye/term protokolü, önek-sonek diff, basit OT konum kaydırma; `CollabSession`: `QWebSocketServer` host + zamanuyumsuz peer join, imleç/metin/terminal yayını, rev uyuşmazlığında sync), görünüm profili (`UiProfile`: tema/accent/tipografi/düzen dışa-içe aktarma, yalnızca görsel alanlar), karşılama ekranında canlı demo kartı (`WelcomeView::buildDemoMode`).
- **Stage 19 — Süreklilik & Bakım ✅ (yapıldı):** sürüm takibi (`GitVersionManager`: `git describe`/kısa-hash sürüm, `lastCommitMessage`, opt-in `createTag`, `SettingsManager`'da `app/version` + `app/firstRun`), önbellek temizleyici (`CacheCleaner`: `~/.cache/clangd` + proje `.clangd` + `*.swp/*.bak/*.tmp`; `.git`'e dokunmaz), iş parçacığı izleyici (`ThreadMonitor`: `QFutureWatcherBase` tabanlı calisiyor/bitti/iptal + `cancelAll`), dil desteği (`LanguageSupport`: uzantı→dil, klasör taramada baskın dil, geçerli boş `compile_commands.json` üretimi), `test_stage19` (55 kontrol: zincir/birleştirme/eklenti-izini/canlılık + localhost WebSocket turu).
- **Stage 20 — Karşılama + Yeni Dosya Akışı ✅ (yapıldı):** hover kaybolma düzeltmesi (kök neden: `withAlphaF().name()` alfayı düşürüp opak metin rengi basıyordu → `ThemeTokens::css()` ile `#aarrggbb`, tüm QSS yarı-saydam renkleri + `WelcomeView` eventFilter renk sabitlemesi), yeni dosyada dil seçimi (`NewFileDialog`: 20 dil + Düz metin, süzme kutusu, son 3 dil üstte), dile göre iskelet (`main()`/`__main__`/`DOCTYPE`…; `editor/fileTemplate` ayarıyla kapatılabilir), kaydetmede dile uygun ad + filtre önerisi (`Adsız-1.py`), sekme başlığında dil rozeti (`Adsız-1 • py`), dil çipi tıklayınca `file.language` (açık sekmenin dilini değiştirir), içi boş sekme sessiz kapanır, uzun alt yazılara `ElideLabel` (…), `test_stage20` (dil tablosu/iskelet/başlık/son-diller).
- **Stage 21 — Üretkenlik Paketi ✅ (yapıldı):** ilk-çalıştırma sihirbazı (`FirstRunDialog`: dil/tema/kısayol profili/font; `m_firstRun` kapanır), kurtarma bildirimi (yedek varsa toast → Zaman Çizelgesi), galeride son özel renkler (5 swatch, kalıcı), palet skoru (birebir/önek/kelime-başı bonusu; ★ + kısayol vardı), sekmede orta-tık kapat + çift-tık sabitle (`◆`, sabit korunur), pano halkası (`ClipboardRing` + `Ctrl+Shift+V` döngü), satır sonlarına imleç (`Ctrl+Shift+L`, 300 kap), büyük dosyada LSP erteleme (tamamı yüklenince başlar), dile göre sözlük (kod→önce en_US), EOL çipi çevirir (LF↔CRLF, tek geri-al), kodlama bilgi penceresi, gezginde F2/Del/F5, arama geçmişi (tamamlama+kalıcı) + önizleme vurgusu, terminal geçmişi (↑/↓+kalıcı) + 📁 dizin takibi, commit şablonu + amend, AI sohbet MD dışa aktarma, bütçe aşım uyarısı (3 trim noktası), kısayol çakışma uyarısı (`ShortcutCheck` + Ayarlar satırı), minimap satır-merkezli atlama + boyut tazeleme, `test_stage21` (halka/çakışma/skor/bütçe/şablon).

## Proje yapısı
```
src/main.cpp  src/MainWindow.*        → activity bar + side panel + sekmeler + status
src/core/SettingsManager.*            → QSettings kalıcılığı
src/core/ThemeManager.*               → token hattı: palet + accent + tipografi → QSS
src/core/ThemeTokens.*                → JSON tema paleti + renk yardımcıları
src/core/ThemeStore.*                 → gömülü/özel tema deposu + içe/dışa aktarma
src/core/QssBuilder.*                 → token'lardan tam uygulama QSS'i üretir
src/core/AccentColor.*                → vurgu rengi presetleri + hover/pressed/soft türevleri
src/core/UiMetrics.*                  → boşluk/yarıçap/ölçek ölçü tokenları
src/core/Typography.*                 → UI+editör fontu, satır yüksekliği, ligature
src/core/IconTheme.*                  → SVG ikonları temaya göre renklendirir (Qt6::Svg)
src/widgets/ThemeGalleryDialog.*      → tema galerisi + accent seçici + JSON aktarım
src/core/Animator.*                   → fade/slide/geometri animasyonları + azaltılmış hareket
src/core/ToastManager.*               → sağ altta yığılan toast bildirim overlay'i
src/core/LayoutPresets.*              → Standart/Editör/Zen yerleşim şablonları
src/core/UiProfile.*                  → görünüm profili JSON dışa/içe aktarma
src/core/WordDiff.*                   → LCS tabanlı kelime diff vurgusu
src/widgets/WelcomeView.*             → karşılama ekranı: hızlı eylem + son dosyalar
src/widgets/EmptyState.*              → yeniden kullanılabilir boş durum görünümü
src/core/BracketDepth.*               → gökkuşağı parantez derinlik analizi + palet
src/core/ScopeChain.*                 → yapışkan kaydırma için kapsam zinciri
src/core/SearchMarks.*                → editör-içi arama isabetleri (FindHit)
src/core/DiffGutter.*                 → git diff satır durumları (gutter + minimap)
src/core/HoverCard.*                  → zengin LSP hover kartı HTML üretici
src/core/SelectionGrow.*              → akıllı seçim genişletme mantığı
src/core/CompletionList.*              → LSP tamamlama parse + filtre/sırala
src/core/SymbolTree.*                 → document/workspace sembol ağacı + düzleştirme
src/core/LocationSet.*                → referans konumları parse + dosyaya göre grup
src/core/TextEdits.*                  → TextEdit/WorkspaceEdit uygula + önizleme
src/core/CodeActionList.*             → code action parse + kind filtre
src/core/SignatureHelp.*              → imza yardımı parse + render
src/core/InlayHintList.*              → satır içi ipucu parse
src/core/SemanticTokens.*             → semantik token delta-decode + rol eşleme
src/core/CallHierarchyTree.*          → prepare/incoming/outgoing çağrı ağacı
src/widgets/CompletionPopup.*         → imleç altı tamamlama listesi
src/widgets/SymbolPickerDialog.*      → outline + çalışma alanı sembol seçici
src/widgets/ReferencesDialog.*        → gruplu referans ağacı + atlama
src/widgets/RenamePreviewDialog.*     → yeniden adlandırma önizleme + onay
src/widgets/CallHierarchyDialog.*     → gelen/giden çağrı ağacı
src/core/MiParser.*                   → GDB/MI kayıt çözümleyici
src/core/BreakpointStore.*            → kesme noktaları kalıcılık (dosya:satır)
src/core/TestParser.*                 → ctest/gtest/pytest çıktı çözümleyici
src/core/TestDiscovery.*              → çalıştırıcı bulma + listeleme komutları
src/core/GcovParser.*                 → .gcov kapsama çözümleyici
src/core/ProblemMatcher.*             → derleme/test çıktısı → tanı listesi
src/core/LaunchConfig.*               → .verso/launch.json yükle/kaydet
src/core/GdbDriver.*                  → GDB/MI2 sürücü (kesme/adım/yığın/değişken)
src/widgets/DebugPanel.*              → hata ayıklama araç çubuğu + 4 sekme
src/widgets/TestExplorer.*            → test ağacı + çalıştır + günlük
src/core/GhostCompletion.*            → FIM istemi + hayalet temizliği + tetik
src/core/InlineEdit.*                 → tarif + seçim → kod çıkarımı
src/core/DocGen.*                     → fonksiyon başı bulma + belge istemi
src/core/TestGen.*                    → test hedefi çözümleyici + test istemi
src/core/CommitMsg.*                  → conventional doğrulama + commit istemi
src/core/PromptLibrary.*              → özel istem deposu
src/core/ContextBudget.*              → token tahmini + bağlam kırpma
src/widgets/PromptLibraryDialog.*     → istem kitaplığı diyaloğu
src/core/ConnectionProfile.*          → uzak bağlantı profili (host/port/anahtar/jump/TOFU)
src/core/SshSession.*                 → SSH oturumu (exec/sftp/oku/yaz, komut kurma)
src/core/RemoteFileSystem.*           → ssh:// URI + GNU ls/stat ayrıştırma
src/core/RemoteTaskRunner.*           → uzak derleme çıktısı → sorun listesi
src/core/LspTransport.*               → yerel/uzak LSP süreç kurma (ssh stdio köprüsü)
src/core/GitRunner.*                  → yerel/uzak git çalıştırıcı
src/core/PortForwarder.*              → ssh -L tünel yöneticisi
src/widgets/RemoteConnectDialog.*     → profil + bağlan diyaloğu
src/widgets/RemoteExplorer.*          → uzak dosya ağacı
src/widgets/RemoteTerminal.*          → uzak etkileşimli kabuk
src/widgets/PortForwardPanel.*        → aktif tüneller tablosu
src/core/SnippetEngine.*              → snippet gövde ayrıştırma + genişletme
src/core/SnippetManager.*             → gömülü + özel snippet deposu
src/core/FoldingRanges.*              → LSP foldingRange normalize/birleştirme
src/core/BlockSelect.*                → dikdörtgen seçim hesabı
src/core/AutoPairs.*                  → çift kapatma/sarma/akıllı Enter kararları
src/core/PasteTransform.*             → özel yapıştırma dönüşümleri
src/core/OutlineFallback.*            → regex sınıf/fonksiyon taraması
src/core/LocalHistory.*               → zaman damgalı anlık görüntüler + budama
src/core/ReplaceEngine.*              → hariç glob + dosya-başı değiştirme + durum koruma
src/core/WordComplete.*               → açık belge kelime adayları
src/widgets/OutlinePanel.*            → sembol ağacı paneli
src/widgets/TimelinePanel.*           → geçmiş önizleme/fark/geri yükleme
src/widgets/SnippetDialog.*           → snippet paleti
src/widgets/EditorFindBar.*           → Ctrl+F bulma çubuğu + sayaç + gezinme
src/core/Density.*                    → arayüz yoğunluğu (kompakt/rahat/geniş)
src/core/ColorBlind.*                 → renk körü dostu palet dönüşümü
src/core/AutoTheme.*                  → sistem/zamanlanmış otomatik tema kararı
src/core/ProfileStore.*               → isimli görünüm profili deposu
src/core/ThemeValidator.*             → tema JSON doğrulama + AI yanıt çıkarımı
src/widgets/ThemeEditorDialog.*       → canlı tema editörü + AI tema üretici
src/widgets/TitleBar.*                → frameless özel pencere başlığı
resources/themes/high-contrast.json   → erişilebilirlik teması (11 hazır tema)
resources/themes/*.json              → 11 hazır tema paleti (QSS yerine token)
resources/icons/*.svg                → 16 Feather-tarzı ikon (currentColor tabanlı)
src/core/LanguageManager.*            → TR/EN sözlük
src/core/OllamaClient.*               → /api/tags + /api/chat
src/widgets/CodeEditor.*              → gutter + highlighter + load/save
src/widgets/ExplorerPanel.*           → QFileSystemModel ağacı + .gitignore filtresi + ikonlar + çöp
src/widgets/GitPanel.*                → git CLI: status/commit/push/pull + hunk diff
src/widgets/DiffDialog.*              → unified diff çözümleyici + hunk stage/discard
src/widgets/GitTools.*                → geçmiş/stash/uzak sekmeleri
src/widgets/TodoPanel.*               → TODO/FIXME tarama (Qt Concurrent)
src/widgets/CompareDialog.*           → klasör karşılaştırma + dosya diff
src/core/GitIgnore.*                  → .gitignore glob → regex eşleştirici
src/core/TrashManager.*               → güvenli silme / geri yükleme
src/core/ProjectSessions.*            → proje oturumu + anlık görüntüler
src/core/FileIcons.*                  → uzantı bazlı renkli ikonlar
src/core/AgentTools.*                 → ajan araçları (read/write/list/search/run) + sandbox
src/core/AgentLoop.*                  → çok adımlı plan→araç→gözlem döngüsü
src/core/PatchQueue.*                 → çoklu dosya düzenleme kuyruğu + unified diff
src/core/ContextResolver.*            → @dosya/@seçim/@sorunlar/@proje etiketleri
src/core/ChatStore.*                  → çoklu sohbet oturumu (JSON kalıcılık)
src/core/TokenStats.*                 → token sayacı + yaklaşık maliyet
src/widgets/PatchReviewDialog.*       → ajan düzenlemelerini incele & toplu uygula
src/core/StartupArgs.*                → komut satırı argümanları (--line/--new/--command)
src/core/ToolchainProbe.*             → harici araç zinciri tespiti + sürüm
src/core/TaskRunner.*                 → tasks.json görev çalıştırıcı (QProcess)
src/core/TaskChain.*                  → dependsOn zinciri + input genişletme + problemMatcher
src/core/PluginEngine.*               → eklenti API v1 (QJSEngine + fs.read/fs.write izni)
src/core/CollabMerge.*                → işbirliği protokolü + diff + basit OT
src/core/CollabSession.*              → WebSocket host/peer oturumu (imleç/metin/terminal)
src/core/UiProfile.*                  → görünüm profili dışa/içe aktarma
src/core/GitVersionManager.*          → git describe sürüm + opt-in tag
src/core/CacheCleaner.*               → clangd + geçici dosya temizliği
src/core/ThreadMonitor.*              → QFutureWatcher tabanlı iş izleyici
src/core/LanguageSupport.*            → dil algılama + compile_commands.json + yeni dosya dilleri (21 kayıt/iskelet/son diller)
src/widgets/NewFileDialog.*           → yeni dosya dil seçici (süzme + son kullanılanlar)
tests/test_stage19.cpp                → Stage 18/19 birim + canlı localhost testleri
tests/test_stage20.cpp                → Stage 20 dil tablosu/iskelet/başlık testleri
tests/test_stage21.cpp                → Stage 21 halka/çakışma/skor/bütçe/şablon testleri
tests/test_stage22.cpp                → Stage 22 kontrast/çeviri kapsama testleri
src/core/A11yCheck.*                  → WCAG kontrast oranı + tema denetimi (saf mantık)
src/widgets/ShortcutDialog.*          → kısayol hile sayfası (süzmeli, salt okunur)
src/widgets/FirstRunDialog.*          → ilk-çalıştırma sihirbazı
tests/test_stage23.cpp                → Stage 23 ızgara ölçüm testleri
src/core/GridCheck.*                  → monospace ızgara denetimi (saf mantık)
tests/test_stage24.cpp                → Stage 24 sembol/metrik/kopya/blame/todo testleri
tests/test_stage25.cpp                → Stage 25 istem/makro/araç testleri
tests/test_stage26.cpp                → Stage 26 MI/launch testleri
tests/test_stage27.cpp                → Stage 27 sunucu/venv/URI testleri
src/core/LspServers.*                 → dil sunucusu kayıt tablosu + venv çözümleme
tests/test_stage28.cpp                → Stage 28 yer imi/yorum/girdi/shebang testleri
src/core/BookmarkStore.*              → yer imleri (kalıcı, sonraki/önceki)
src/widgets/BookmarkDialog.* + MarkdownPreviewDialog.* + ImageViewerDialog.* + HexViewDialog.* + GitHistoryDialog.* + GitTagsDialog.* → yeni diyaloglar
tests/test_stage29.cpp                → Stage 29 eklenti v2 testleri
src/widgets/PluginManagerDialog.*     → eklenti yöneticisi (kur/izin/karantina/günlük)
belgeler/eklenti-api.md               → eklenti API v2 kılavuzu
tests/test_stage30.cpp                → Stage 30 birleştirme/ssh/pano/zincir testleri
src/core/MergeParse.* + SshConfig.*   → çakışma ayrıştırma + ssh-config içe aktarma
src/widgets/MergeEditorDialog.* + SearchEditorDialog.* + NewProjectDialog.* + ClipboardDialog.* → yeni diyaloglar
tests/test_stage31.cpp                → Stage 31 bütünlük/geri çekilme/rotasyon/kota/fuzz testleri
src/core/Stability.* + CrashHandler.* → bütünlük mührü, geri çekilme, log rotasyonu, çökme izi
tests/test_stage32.cpp                → Stage 32 hız yardımcıları/artımlı RAG/ters indeks testleri
tests/test_stage33.cpp                → Stage 33 yetenek/görü/gömme/hibrit/puan/özet/dallanma testleri
src/core/ModelCapabilities.* + ImageUtil.* → model yeteneği + görsel hazırlama
src/core/Embedding.* + EmbeddingClient.* + HybridRanker.* → vektör matematiği + gömme istemcisi + RRF
src/core/ResponseScorer.* + ConversationSummarizer.* → yanıt kalite puanı + sohbet özeti
src/widgets/ModelArenaDialog.*        → çok-modelli arena
tests/test_stage34.cpp                → Stage 34 politika/bütçe/beceri/bellek/refleksiyon/otonomi testleri
src/core/AgentPolicy.* + AgentBudget.* → ajan politika ve çalışma bütçesi motorları
src/core/SkillRegistry.* + SkillChain.* → beceri kayıt defteri + zincir planlayıcı
src/core/AgentMemory.* + AgentReflection.* → ajan belleği + öz-değerlendirme
src/core/ProjectHealth.* + AgentRunStore.* → proje sağlık skoru + koşu günlüğü/geri alma
src/widgets/AgentPanelDialog.*         → ajan paneli (durum/beceri/bellek/günlük)
tests/test_stage35.cpp                → Stage 35 sağlayıcı/kasa/kodlama/SSE/retry testleri
src/core/ai/LlmProvider.* + ProviderPrefs.* → sağlayıcı kayıt defteri + sağlayıcı tercihleri
src/core/ai/SecretStore.*             → API anahtarı kasası (keyring → 0600 dosya → env)
src/core/ai/AiMessage.* + LlmClient.* → birleşik mesaj modeli + sağlayıcıdan bağımsız istemci
src/core/ai/providers/ProviderCodec.* + SseParser.* → 4 sağlayıcı gövdesi + paylaşılan SSE
src/core/ai/AiToolBridge.*            → native araç çağırma ↔ metin protokolü köprüsü
src/core/ai/ProviderPricing.* + UsageLedger.* → fiyatlandırma, kalıcı kullanım geçmişi, kota
src/core/ai/ProviderHealth.*            → sağlık skoru + otomatik failover adayları
src/core/ai/TaskRouter.*                → görev sınıflandırma + sağlayıcı/model yönlendirme
src/core/ai/EmbedBridge.* + EmbedCache.* → sağlayıcı-duyarsız gömme + LRU vektör önbelleği
src/core/ai/AgentLlmAdapter.*           → ajan döngüsü için sağlayıcı-duyarsız LLM
src/core/ai/ProviderBench.*             → sağlayıcı mikro-karşılaştırması
src/core/PerfTools.*                  → ikili sez, boyut süzgeci, LRU önbellek, ters indeks
- **Stage 36 — Sağlayıcı Derinliği, Ekonomi & Kota ✅ (yapıldı):** sağlayıcı+model bazlı **fiyatlandırma** (`ProviderPricing` — OpenAI/Claude/Gemini/Groq/DeepSeek/Mistral/xAI tabloları, `:free`/yerel/NIM geliştirme kredisi **her zaman 0 USD**, önbellekten okunan giriş ~%10 indirim, toplu işlem %50, bilinmeyen model "fiyat bilinmiyor" uyarısı), **kalıcı kullanım geçmişi + kota** (`UsageLedger` — `~/.verso/ai-usage.json`, gün/dağılım toplamları, 90 gün budama, sağlayıcı başına günlük istek/token tavanı + "kota doldu" erken reddi), `TokenStats` v2 (sağlayıcı farkında maliyet + `ai/costGuardUsd` **koşu maliyet onayı**), **sağlayıcı sağlık skoru** (`ProviderHealth` — hata oranı/ortalama gecikme/kalıcı, 3 üst üste hatada "bozuk") ve **otomatik failover** (`401/403/404/5xx` → yeteneği eşdeğer, anahtarı olan sağlayıcıya tek seferlik devir; 429 yalnız yeniden denenir), **görev tabanlı yönlendirme** (`TaskRouter` — önemsiz/basit/orta/karmaşık/ajan/görsel/gömme sınıflama, hızlı↔güçlü sağlayıcı eşlemesi, görsel desteklemeyene otomatik geçiş, "gömme sağlayıcısı ≠ sohbet sağlayıcısı", manuel kilit), **sağlayıcı-duyarsız gömme** (`EmbedBridge` — Ollama yolu korunur, NIM `nv-embedqa-e5-v5`/OpenAI/Gemini/UnoRouter gömme modelleri de RAG'da çalışır; Ollama'da toplu gömme hatası giderildi) + `EmbedCache` (LRU, aynı metin için tekrar ağ çağrısı yok), `ModelCapabilities` ↔ `ProviderSpec` birleşimi (yeni **düşünme** ve **ücretsiz** rozetleri, sağlayıcı bağlam tavanı), ajan döngüsü için **`AgentLlmAdapter`** (Ollama dahil her sağlayıcıda tek yol; kota denetimi + sağlık/kullanım kaydı + native araç çağırma → `<tool_call>` geri uyumu; `AgentLoop::StepMeta` ile adım telemetrisi), Ayarlar → AI Sağlayıcıları'na **maliyet tavanı / kota / yönlendirme / sağlık** bölümü + **Sağlayıcıları Karşılaştır** mikro-bench'i (`ProviderBench`: kalite %60 + gecikme %25 + maliyet %15, ücretsiz olan yakınsa önerilir), `test_stage36` (362 kontrol, yerel sahte sunucuyla canlı failover testi dâhil).

- **Stage 35 — Çok Sağlayıcılı AI ✅ (yapıldı):** sağlayıcı kayıt defteri (`LlmProvider`: 20 ön ayar — Ollama, **NVIDIA NIM** + yerel NIM, **UnoRouter** + Anthropic/Gemini geçitleri, OpenAI, Claude, Gemini, Groq, OpenRouter, DeepSeek, Mistral, xAI, Together, Azure, LM Studio, llama.cpp, vLLM, özel sunucu; `Quirks` bayraklarıyla sağlayıcı tuhaflıkları veri olarak koddan çıkarıldı: `max_tokens` zorunluluğu, `:free` ücretsiz model, `nvidia/` önek zorunluluğu, `reasoning_effort`, çoklu protokol geçidi), gizli anahtar kasası (`SecretStore`: OS anahtar deposu → 0600 dosya → `VERSO_AI_KEY_*` ortam değişkeni; maskeli gösterim, anahtarlar ayara/loga yazılmaz), birleşik mesaj modeli (`AiMessage`/`AiChunk`/`AiUsage`/`AiToolCall`), dört gövde üretici–çözümcü (`ProviderCodec`: OpenAI-uyumlu, Anthropic Messages, Gemini `generateContent`, Ollama; araç şemaları, görsel `inlineData`/`image_url`, `reasoning_content`/`thinking`), paylaşılan SSE ayrıştırıcı (`SseParser`: parça bölünmesi, çok satırlı `data:`, `[DONE]`, CRLF), birleşik istemci (`LlmClient`: akış + akışsız, model kataloğu, gömme, **429/5xx için üstel geri çekilme + Retry-After**, sağlayıcıya özel Türkçe hata mesajları — NIM'de `nvidia/` öneki ve `max_tokens` ipuçları dâhil), araç çağırma köprüsü (`AiToolBridge`: native function-calling → `<tool_call>` metin protokolüne geri uyum dönüşümü), sağlayıcı tercihleri (`ProviderPrefs`), Ayarlar'da **AI Sağlayıcıları** sekmesi (sağlayıcı/base URL/anahtar/model + "Modelleri Getir" + "Bağlantıyı Test Et" + yetenek rozetleri) ve AI panelinde sağlayıcı seçici, `test_stage35` (308 kontrol).

- **Stage 34 — AI Otonomisi ✅ (yapıldı):** politika motoru (`AgentPolicy`: araç izinleri, salt-okunur küme, otonom kip yalnız okuma + onay zorunluluğu), bütçe motoru (`AgentBudget`: adım/araç/yazma/token/süre kotaları + gerekçeli aşma), beceri kayıt defteri (`SkillRegistry`: 8 dahili beceri + özel beceri kalıcılığı), beceri zinciri (`SkillChain`: hedefe göre seçim + bağımlılık sırası + ilerleme), ajan belleği (`AgentMemory`: kural/yol/komut/başarısızlık notları, tekilleştirme, alakalılık puanı, istem enjeksiyonu), refleksiyon (`AgentReflection`: adım puanı, iyi/tekrar dene/takıldı, araç başarı istatistiği ve ceza satırı), yeni araçlar (`read_range`, `grep_lines`, `find_symbol`, `git_status`, `git_diff`, `run_tests`, `health_scan`), komut güvenliği (tehlikeli kalıp listesi: `rm -rf /`, `curl|sh`, `sudo`, `git push`…), koşu günlüğü (`AgentRunStore` + tek tıkla **geri alma**; sonradan elle değişen dosyalar atlanır), proje sağlık skoru (`ProjectHealth`: TODO/FIXME/uzun dosya/derleme/test → 0-100 + harf + öneriler), otonom hedef ajanı (**varsayılan kapalı**, salt-okunur proje denetimi), ajan paneli (durum/beceri/bellek/koşu günlüğü sekmeleri), ajan ayarları (otonomi, araç/yazma/token/süre tavanı, bellek, beceri, test komutu), `test_stage34` (253 kontrol).
- **Stage 33 — AI Derinliği ✅ (yapıldı):** model yetenek algılama (`ModelCapabilities`: `/api/show` `capabilities` + ad sezgisi; görü/araç/gömme rozetleri), görü (vision) girdisi (`ImageUtil` ölçekle+sıkıştır, `OllamaClient::chatWithImages/chatStreamWithImages` + `showModel/showSync`), AI panelinde görsel ek rozetleri (🖼, boyut tavanı, temizle), gömme istemcisi (`EmbeddingClient` `/api/embed`, eski biçim uyumu), vektör matematiği (`Embedding`: seri/çöz, kosinüs, normalizasyon, ortalama), hibrit sıralayıcı (`HybridRanker` RRF + ağırlıklı birleşim), RAG v2 anlamsal+hibrit (`RagIndexer::setEmbedder/embedAll/queryHybrid`; gömme modeli yoksa anahtar kelimeye düşer), yanıt kalite puanı (`ResponseScorer` sezgisel+kullanıcı oyu), istem galerisi (`PromptLibraryDialog`: arama/önizleme/düzenleme/içe-dışa aktarma JSON + `PromptVars` genişletme), çok-modelli arena (`ModelArenaDialog` eşzamanlı koşu + süre/token/kalite), sohbet dallanma (`ChatStore` `id/parent` ağacı, `pathTo/childrenOf/leafIds`, ⑂ dal seçici), oturum özeti (`ConversationSummarizer` eşik/özet/birleştirme; AI panelinde bağlam sıkıştırma), AI ayarları (`ai/vision`, `ai/embedModel`, `ai/keepAlive`, `ai/parallel`, `ai/summaryTokens`), `test_stage33`.
- **Stage 32 — Hız ✅ (yapıldı):** arama çok çekirdekli (`blockingMapped` + thread havuzu), ikili sez + 4 MB boyut tavanı, RAG artımlı indeksleme (mtime önbelleği) + ters indeksten aday daraltma, ayar önbelleği (SettingsManager), ikon önbelleği (QCache), minimap çubuk katmanı önbelleği (QPixmap), gezgin `uniformRowHeights` + sembolik bağ/özel ikon optimizasyonu, terminal/görev çıktısında blok tavanı (20k), büyük dosya kipi eşikleri (`largeFileMode`), başlangıç bütçesi bildirimi (4000 ms), `test_stage32`.
- **Stage 31 — Soğukkanlılık ✅ (yapıldı):** oturum bütünlük mührü + güvenli kip, ayar şema + çökme raporu anahtarı, bellek tavanı (8 MB salt-okunur), izleyici taşma koruması, LSP fırtına koruması (üstel geri çekilme), eklenti yavaş rozeti, ağ zaman aşımları, çökme izi (opt-in, backtrace, `-rdynamic`), yedek kotası (50 MB/90 gün), RAG iptal yolu, sızıntı taraması (temiz), `test_stage31` (400 tohumlu MI fuzz dahil).
- **Stage 30 — İş Akışı Derinliği ✅ (yapıldı):** 3-yönlü birleştirme düzenleyicisi, arama düzenleyicisi (işaretle + toplu değiştir), proje şablonları (C++/Python/Qt sihirbazı), dosya şablonları (`*.tpl` + değişkenler), terminal çoklu sekme + kabuk profilleri, SSH config içe aktarma, eklenti `fetch` (`net` izni), pano yöneticisi (sabitleme), sık kullanılan komutlar (palet sıklık bonusu), clang-format yedeği, görev zinciri koşturma, uzak yeniden bağlanma, sohbetlerde arama, zamanlanmış AI denetimi, `test_stage30`.
resources/plugins/selam.js + notal.js → örnek eklenti paketi
- **Stage 29 — Eklenti API v2 + Yerel Galeri ✅ (yapıldı):** dosya olayları (`save/open/startup/language`), durum çubuğu, `quickPick`/`inputBox`, terminal gönderme, tanılama katkısı (Sorunlar paneli), görünüm kaydı (HTML pencere), tema içe aktarma, çakışma denetimli tuş katkısı, çift yönlü komut (`execCommand`), `globalState`/`workspaceState`/`get-setConfig`, izinler (`fs/events/ui`) + yönetici geçersiz kılma, bütünlük mührü (sha256 TOFU), 3-hatada karantina, yönetici diyaloğu (kur/kaldır/aç-kapa/izin/ayar/günlük), Teşhis eklenti sekmesi, 3 örnek eklenti, `test_stage29`.
- **Stage 28 — Editör Tamamlama & Güven ✅ (yapıldı):** çalışma alanı güveni (sor/bir-seferlik/kısıtlı; görev/eklenti/ajan kapısı), yer imleri (gutter şeridi + liste + `Ctrl+K Ctrl+B/N/P`), satır yorumu (`Ctrl+/`, `<!-- -->` destekli), Markdown önizleme (senkron), görüntü/hex görüntüleyici, yol kopyalama (3 çeşit), import düzenleme, git dosya geçmişi + etiket yönetimi, terminalde bul, bildirim geçmişi, son projeler kartı, görev girdileri (pickString/confirm) + izleme görevleri, çalıştırılabilir dosyayı hata ayıklama, AI geri alma (20 derinlik), otomatik yeniden yükleme, shebang/uzantısız dil, `test_stage28`.
- **Stage 27 — Dil Sunucusu Derinliği ✅ (yapıldı):** sunucu kayıt tablosu (C++/Python/Rust/Go/JS/TS/Java) + venv öncelikli pylsp + kurulum yönergeleri, çökme kurtarma (bildirim + belge geri-açma), `workspaceFolders` + ilişkili tanı, didChange erteleme (400 ms), tamamlama dokümantasyonu (resolve balonu), imza overload döngüsü, belge bağlantısı açma, renk kutuları (tıkla-düzenle), code lens gönderme sayıları, LSP seçim aralığı (yerel yedekle), tip hiyerarşisi, sembol skor + tür simgeleri, çekme tanılama + ilişkili bilgi, aralık biçimlendirme, LSP trafik günlüğü, sunucu yöneticisi (sürüm + yönerge), `.clangd` izleme + yapılandırma bildirimi, `test_stage27`.
src/core/GdbDriver.*                  → + izleme/bellek/disas/thread/attach/core/yedekleme MI
- **Stage 26 — Hata Ayıklayıcı Olgunluğu ✅ (yapıldı):** değişken hover balonu, izleme paneli (varobj), değer düzenleme (panel + komut), yazmaç/bellek/disas/thread sekmeleri, koşul + hit-count düzenleyici, fonksiyon kesmesi (kalıcı), watchpoint (r/rw), sürece bağlanma, core dump, thread değiştirme, akıllı adım (std atlama), konsol geçmişi + tamamlama, launch.json `${değişken}` + şema uyarısı, zorunlu preBuild (hata eşiği), kaynak eşleme, lib frame filtresi, çıkış özeti (kod+süre), proje geneli adlandırmada dosya dışlama, `test_stage26`.
src/core/WorkspaceSymbols.*           → regex sembol dizini + gönderme sayımı
src/core/CodeMetrics.* + DuplicateFinder.* + GitBlame.* → metrik/kopya/blame ayrıştırma
src/core/PromptVars.* + MacroRecorder.* + ExternalTools.* → şablon/makro/araç çekirdeği
src/widgets/SymbolSearchDialog.*      → Ctrl+T sembol arama
src/widgets/MetricsDialog.*           → Proje Radarı (metrik/kopya/kullanılmayan/bağımlılık)
src/widgets/BulkRenameDialog.*        → desenli toplu adlandırma + önizleme
- **Stage 24 — Proje Radarı & Kod Gezintisi ✅ (yapıldı):** sembol arama (`Ctrl+T`, regex dizin), referans önizleme bölmesi, çağrı hiyerarşisinde derinlik (4) + döngü koruması, satır içi blame hayaleti + ısı haritası gutter'ı, tek-tık önizleme sekmesi (çift-tık kalıcılaştırır), sekme grubu renkleri, proje notları (`.verso/notes.md`), TODO `@kullanıcı`/`!p1-3` + öncelik sıralaması, Proje Radarı (metrik/kopya/kullanılmayan/bağımlılık + atlama), proje geneli yeniden adlandırmada dosya dışlama, `test_stage24`.
- **Stage 25 — AI Derinliği & Otomasyon ✅ (yapıldı):** ajan plan kapısı (onaylı başlatma), çift model karşılaştırma (A akar + B akışsız), istem şablon değişkenleri (`{{selection/file/date}}`), kaydetmede ilgili test (ayarlı + Test gezgini anahtarı), staged diff AI incelemesi, yanıt geri alma (dallanma), token bütçesi (ayar + uyarı), komut makroları (kaydet/oynat, kalıcı), toplu adlandırma, snippet `$CURRENT_DATE/$TM_AUTHOR`, zamanlanmış görevler (`scheduleMin`), harici araçlar (`.verso/tools.json` → `tool.*`), model indirme kuyruğu, AI çevrimdışı rozeti, `test_stage25`.
- **Stage 23 — Piksel Izgara + Editör Görünümü ✅ (yapıldı):** ızgara garantisi (`GridCheck`: ASCII advance eşitliği, `iere`/`teat` piksel kriteri; değilse monospace'e dönüş + `Izgara ✓/✗` çipi), imleç stili (çubuk/blok/alt çizgi, özel çizim + yanıp sönme), yumuşak kaydırma, satır vurgusu + seçim opaklığı, parantez stili (renk/zemin/alt çizgi), satır sonu işaretleri, katlama oku (sol/sağ/gizli), editör yazı tipi diyaloğu (`editorFontFamily`), satır yüksekliği + harf aralığı kaydırıcıları, minimap genişliği, galeride renk körü önizlemesi, `test_stage23`.
resources/plugins/echo.js             → örnek eklenti (ilk açılışta kopyalanır)
belgeler/kullanim.md                  → kullanıcı kılavuzu
CHANGELOG.md                          → sürüm değişiklik günlüğü
- **Stage 22 — Sürüm Olgunluğu ✅ (yapıldı):** sürüm 1.4.0 + değişiklik günlüğü, Teşhis'e git sürümü + erişilebilirlik sekmesi (tema kontrastı/çeviri kapsama) + tanı raporu dışa aktarma, kısayol hile sayfası (`Yardım` menüsü), fabrika ayarları (JSON yedekli sıfırlama), eklenti motoru bağlandı (gecikmeli yükleme + `plugin.*` palet komutları + klasörü aç + örnek `echo.js`), tasks.json oluşturma onayı, SSH parmak izi düğmesi, Ollama kurulum kılavuzu, başlangıç ölçüm işaretleri, AppData/AppImage doğrulaması (ikona SVG + dönüştürme), `test_stage22` (kontrast/çeviri).
src/core/ClipboardRing.h              → pano halkası (saf veri yapısı)
src/core/ShortcutCheck.h              → kısayol çakışma denetimi (saf mantık)
src/widgets/FirstRunDialog.*          → ilk-çalıştırma sihirbazı
src/core/PerfMonitor.*                → başlangıç/yükleme süreleri + RSS bellek
src/core/KeymapPresets.*              → VS Code / JetBrains / Vim kısayol profilleri
src/core/BackupManager.*              → zaman damgalı yedekler + kurtarma
src/core/WorkspaceConfig.*            → .verso/workspace.json proje ayarları
src/core/SettingsIO.*                 → ayarları JSON dışa/içe aktarma
src/core/AboutInfo.*                  → sürüm + yenilikler + güncelleme manifesti
src/widgets/TaskPanel.*               → görev listesi + çalıştır/durdur + canlı çıktı
src/widgets/DiagnosticsDialog.*       → Sistem Teşhisi (Hakkında/Araç/Performans/Yedek)
packaging/                            → .desktop + appdata + AppImage betiği
src/widgets/AiPanel.*                 → chat UI + options (num_ctx/num_gpu/...)
src/widgets/SettingsDialog.*          → dil + tema + Ollama formu
resources/themes/{dark,light}.qss
```
