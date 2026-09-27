# Verso Coder — pacman deposu (AUR hesabı olmadan)

AUR hesabı açılamıyorsa resmi olmayan depo buradan kurulur.

## Kurulum

`/etc/pacman.conf` sonuna ekle:

```
[verso]
SigLevel = Optional TrustAll
Server = https://github.com/versochar/verso-coder/releases/download/repo
```

Sonra:

```
sudo pacman -Sy verso-coder
```

## Güvenlik notu (dürüst)

- Depo **imzasızdır** (`TrustAll`). Paketler GitHub Actions + bakımcı
  makinesinde derlenir; SHA256 toplamları release sayfasındadır.
- AUR hesabı açıldığında resmi `verso-coder` paketi gelecek; o zaman bu
  depoyu kaldırıp AUR'a geçin (dosya çakışması olmaz, aynı yollar).

## Bakımcı için güncelleme

```
./packaging/repo/update-repo.sh /tmp/opencode/pkgbuild
# çıkan verso.db* + *.pkg.tar.zst dosyalarını 'repo' etiketli release'e yükle
```
