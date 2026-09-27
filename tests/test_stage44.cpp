// Stage 44 testleri: sahte ssh/sftp ikililerine karşı uzak katman.
// PATH başına konan betikler gerçeğini taklit eder; ağ yok, saf.
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QProcessEnvironment>
#include <QSettings>
#include <QElapsedTimer>
#include <QSettings>
#include <QTemporaryDir>
#include <cstdio>

#include "../src/core/ConnectionProfile.h"
#include "../src/core/SshSession.h"

static int g_pass = 0, g_fail = 0;

#define CHECK(cond)                                                            \
    do {                                                                       \
        if (cond) {                                                            \
            ++g_pass;                                                          \
        } else {                                                                \
            ++g_fail;                                                          \
            fprintf(stderr, "FAIL %d %s\n", __LINE__, #cond);                    \
            fflush(stderr);                                                    \
        }                                                                      \
    } while (0)

#define CHECK2(cond, why)                                                      \
    do {                                                                       \
        if (cond) {                                                            \
            ++g_pass;                                                          \
        } else {                                                                \
            ++g_fail;                                                          \
            fprintf(stderr, "FAIL %d %s (%s)\n", __LINE__, #cond, why);          \
            fflush(stderr);                                                    \
        }                                                                      \
    } while (0)

// Sahte ssh: "true" → 0; "cat 'yol'" → canned; "base64 -d >" → stdin'i ye, 0;
// "hata" → 1 + stderr; "uyu N" → N sn uyu (zaman aşımı testi).
static const char* kFakeSsh = R"SH(#!/bin/sh
cmd="$*"
case "$cmd" in
  *" true"*) exit 0 ;;
  *" hata"*) echo "uzak hata" >&2; exit 1 ;;
  *" uyu "*) sleep 30; exit 0 ;;
esac
case "$cmd" in
  *cat\ *) echo "UZAK-ICERIK"; exit 0 ;;
  *base64*) cat > /dev/null; exit 0 ;;
esac
echo "SAHTE-SSH: $cmd"
exit 0
)SH";

// Sahte sftp: toplu işi stdin'den okur, put/get satırını dosyaya yazar.
static const char* kFakeSftp = R"SH(#!/bin/sh
LOG="$FAKE_SFTP_LOG"
while IFS= read -r line; do
  echo "$line" >> "$LOG"
done
exit 0
)SH";

static void writeExe(const QString& dir, const QString& name, const char* src) {
    QFile f(QDir(dir).filePath(name));
    CHECK(f.open(QIODevice::WriteOnly));
    f.write(src);
    f.close();
    QFile::setPermissions(f.fileName(),
                          QFile::permissions(f.fileName()) | QFile::ExeOwner);
}

// --- 1: saf komut kurma ---
static void testPureArgs() {
    ConnectionProfile p;
    p.host = "sunucu";
    p.user = "kul";
    p.port = 22;
    CHECK(SshSession::sshTarget(p) == "kul@sunucu");
    QStringList a = SshSession::sshBaseArgs(p);
    CHECK(!a.contains("-p")); // varsayılan port bayraksız
    CHECK(a.contains("BatchMode=yes"));
    CHECK(a.contains("ConnectTimeout=10"));
    p.port = 2222;
    a = SshSession::sshBaseArgs(p);
    CHECK(a.contains("-p") && a.contains("2222"));
    p.keyPath = "/k/id";
    p.jumpHost = "atlama";
    a = SshSession::sshBaseArgs(p);
    CHECK(a.contains("-i") && a.contains("/k/id"));
    CHECK(a.contains("-J") && a.contains("atlama"));
    p.trustNewHosts = true;
    a = SshSession::sshBaseArgs(p);
    CHECK(a.contains("StrictHostKeyChecking=accept-new"));
    ConnectionProfile p2;
    p2.host = "h";
    CHECK(SshSession::sshTarget(p2) == "h"); // kullanıcısız
    // sftp alıntısı
    CHECK(SshSession::sftpQuote("/a/b.txt") == "\"/a/b.txt\"");
    CHECK(SshSession::sftpQuote("/a/\"k\"/b.txt") == "\"/a/\\\"k\\\"/b.txt\"");
    CHECK(SshSession::sftpQuote("C:\\yol") == "\"C:\\\\yol\"");
}

// --- 2: sahte ssh'a karşı bağlantı + komut ---
static void testFakeSsh(const QString& binDir) {
    ConnectionProfile p;
    p.host = "sahte";
    p.user = "u";
    SshSession s;
    s.setProfile(p);
    CHECK(!s.isConnected());
    CHECK2(s.testConnection(5000), "sahte bağlantı kurulmadı");
    CHECK(s.isConnected());
    SshSession::ExecResult r = s.exec("echo merhaba", QString(), 5000);
    CHECK(r.exit == 0);
    CHECK(r.out.contains("SAHTE-SSH"));
    SshSession::ExecResult e = s.exec("hata", QString(), 5000);
    CHECK(e.exit == 1);
    CHECK(e.err.contains("uzak hata"));
    // Zaman aşımı
    QElapsedTimer t;
    t.start();
    SshSession::ExecResult to = s.exec("uyu 30", QString(), 1500);
    CHECK(t.elapsed() < 10000);
    CHECK(to.exit == -1);
    s.disconnect();
    CHECK(!s.isConnected());
}

// --- 3: uzak dosya okuma/yazma ---
static void testRemoteFile(const QString& binDir) {
    Q_UNUSED(binDir);
    ConnectionProfile p;
    p.host = "sahte";
    SshSession s;
    s.setProfile(p);
    SshSession::ExecResult r = s.readFile("/uzak/a.txt", 5000);
    CHECK(r.exit == 0);
    CHECK(r.out.contains("UZAK-ICERIK"));
    // Özel karakterli içerik base64 hattından bozulmadan geçer
    const QString tricky = "tırnak \" ve ' ve $degisken ve `backtick`\nline2";
    SshSession::ExecResult w = s.writeFile("/uzak/y.txt", tricky, 5000);
    CHECK(w.exit == 0);
}

// --- 4: sftp toplu işi ---
static void testSftp(const QString& binDir, const QString& logFile) {
    Q_UNUSED(binDir);
    qputenv("FAKE_SFTP_LOG", logFile.toUtf8());
    ConnectionProfile p;
    p.host = "sahte";
    SshSession s;
    s.setProfile(p);
    QTemporaryDir d;
    QFile f(d.path() + "/yerel.txt");
    CHECK(f.open(QIODevice::WriteOnly));
    f.write("veri");
    f.close();
    CHECK(s.upload(f.fileName(), "/uzak/hedef.txt", 5000));
    CHECK(s.download("/uzak/kaynak.txt", d.path() + "/in.txt", 5000));
    QFile log(logFile);
    CHECK(log.open(QIODevice::ReadOnly));
    const QString lines = QString::fromUtf8(log.readAll());
    CHECK(lines.contains("put \"") && lines.contains("/uzak/hedef.txt"));
    CHECK(lines.contains("get \"") && lines.contains("/uzak/kaynak.txt"));
    CHECK(lines.contains("bye"));
    qunsetenv("FAKE_SFTP_LOG");
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    QTemporaryDir home, binDir;
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, home.path());
    QSettings::setPath(QSettings::IniFormat, QSettings::SystemScope, home.path());
    qputenv("XDG_DATA_HOME", home.path().toUtf8());
    qputenv("HOME", home.path().toUtf8());

    writeExe(binDir.path(), "ssh", kFakeSsh);
    writeExe(binDir.path(), "sftp", kFakeSftp);
    // Sahte ikililer PATH başına
    const QString path =
        binDir.path() + ":" + QString::fromUtf8(qgetenv("PATH"));
    qputenv("PATH", path.toUtf8());

    testPureArgs();
    testFakeSsh(binDir.path());
    testRemoteFile(binDir.path());
    const QString logFile = home.path() + "/sftp.log";
    testSftp(binDir.path(), logFile);

    fprintf(stderr, "STAGE44: %d passed, %d failed\n", g_pass, g_fail);
    fflush(stderr);
    return g_fail == 0 ? 0 : 1;
}
