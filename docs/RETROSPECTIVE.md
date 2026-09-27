# 50 Stage Retrospektifi — Her Stageden Bir Ders

- **35:** Quirk'ler veri olmalı, kod değil (sağlayıcı farkları tabloya).
- **36:** Ekonomi önce gelir (kota/fiyat olmadan failover anlamsız).
- **37:** Tek cephe (12 görev, 1 yol) — çok cephe çok hata demek.
- **38:** Test yazılmadan güvenlik iddiası yapılmaz (PathGuard'ı test buldu).
- **39:** Canlı kota yakmadan canlı kanıt üretilir (yapay yoğunluk + 1 istek).
- **40:** Ölü bağlantılar sessizce çürür (eşik ayarı hiç okunmuyordu).
- **41:** Mevcut kodu okumadan plan yazma (Stage 22 motoru vardı).
- **42:** Zaman aşımı olmayan her istek bir sızıntıdır (LSP handler'ları).
- **43:** Uzak komut alıntısı sonradan eklenmez, ilk gün yazılır.
- **44:** Sahte ikililer ağ testinden ucuzdur (ssh/sftp PATH oyunuyla).
- **45:** Türkçe `İ` küçültmesi her dizgi eşleşmesini bozar (elle katla).
- **46:** `removeTab` sinyal ateşler (Qt varsayılanları okunmalı).
- **47:** Sıfır `accessibleName` = sıfır erişilebilirlik iddiası.
- **48:** Bayrak, ilk QSettings kullanımından önce işlenir (sıra her şeydir).
- **49:** Ölü kod en tehlikeli koddur (CollabSession bağlı değildi).
- **50:** Dürüst sınır listesi, şişirilmiş özellik listesinden değerlidir.

## Süreç kuralları (kalıcı)
1. Her stage: derleme + tam ctest + duman + README + commit + push.
2. Token'lı uzak URL push'tan sonra sade URL'ye alınır.
3. Test bulgusu > plan sadakati (plan revize edilir, gerçek kazanır).
4. Kısa yanıtlar ("a"/"devam") tam iş demektir; bıkkınlık değil güven.
5. `ai-probe` değişiklikten sonra yeniden derlenir (stale .o tuzağı).
6. CMake'e kaynak eklerken gerçek komşulukla eşleştir (yanlış yol tuzağı).
7. Python heredoc içinde C++ `\n` ve `\"` kaçışlarına dikkat (en çok zaman
   yiyen hata sınıfıydı).
