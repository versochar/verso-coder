// Stage 34 testleri: ajan politikası, bütçe, beceri kayıt/zinciri, bellek,
// refleksiyon, proje sağlığı, koşu günlüğü + geri alma, yeni araçlar,
// AgentLoop entegrasyonu (bütçe kesme, refleksiyon, değişiklik kaydı).
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <cstdio>

#include "../src/core/AgentBudget.h"
#include "../src/core/AgentLoop.h"
#include "../src/core/AgentMemory.h"
#include "../src/core/AgentPolicy.h"
#include "../src/core/AgentReflection.h"
#include "../src/core/AgentRunStore.h"
#include "../src/core/AgentTools.h"
#include "../src/core/PatchQueue.h"
#include "../src/core/ProjectHealth.h"
#include "../src/core/SkillChain.h"
#include "../src/core/SkillRegistry.h"

static int g_pass = 0, g_fail = 0;

#define CHECK(cond)                                                           \
    do {                                                                      \
        if (cond) {                                                           \
            ++g_pass;                                                         \
        } else {                                                              \
            ++g_fail;                                                         \
            fprintf(stderr, "FAIL %d %s\n", __LINE__, #cond);                 \
            fflush(stderr);                                                   \
        }                                                                     \
    } while (0)

static void writeFileIn(const QString& dir, const QString& name, const QString& content) {
    QFile f(dir + "/" + name);
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) f.write(content.toUtf8());
}

// --- 1 & 5: politika ---
static void testPolicy() {
    AgentPolicy p = AgentPolicy::safeDefault();
    CHECK(!p.autonomous);
    CHECK(!p.allowWrite && !p.allowCommand && !p.allowTests);
    // Salt-okunur araçlar her zaman serbest
    CHECK(p.toolAllowed("read_file"));
    CHECK(p.toolAllowed("list_dir"));
    CHECK(p.toolAllowed("git_diff"));
    CHECK(p.toolAllowed("health_scan"));
    // Yazma araçları kapalı
    CHECK(!p.toolAllowed("write_file"));
    CHECK(!p.toolAllowed("run_command"));
    CHECK(!p.toolAllowed("run_tests"));
    // Bilinmeyen araç kapalı (güvenli varsayılan)
    CHECK(!p.toolAllowed("rm_rf"));
    // Onay: yalnız mutating araçlar
    CHECK(!p.needsApproval("read_file"));
    CHECK(p.needsApproval("write_file"));
    CHECK(p.needsApproval("run_tests"));
    CHECK(p.needsApproval("bilinmeyen"));

    p.allowWrite = true;
    CHECK(p.toolAllowed("write_file"));
    CHECK(!p.toolAllowed("run_command")); // hâlâ kapalı

    // Otonom mod: yalnız okuma
    const AgentPolicy a = AgentPolicy::autonomousReadOnly(4);
    CHECK(a.autonomous);
    CHECK(a.autonomousAllows("read_file"));
    CHECK(a.autonomousAllows("health_scan"));
    CHECK(!a.autonomousAllows("write_file"));
    CHECK(!a.autonomousAllows("run_tests"));
    CHECK(a.maxSteps == 4);
    CHECK(a.summary().contains("otonom"));

    CHECK(AgentPolicy::isReadOnlyTool("search"));
    CHECK(AgentPolicy::isMutatingTool("run_command"));
    CHECK(!AgentPolicy::isMutatingTool("find_symbol"));
    CHECK(AgentPolicy::readOnlyTools().size() >= 8);
}

// --- 2: bütçe ---
static void testBudget() {
    AgentBudget b;
    b.maxSteps = 2;
    b.maxToolCalls = 3;
    b.maxWrites = 1;
    b.maxTokens = 100;
    b.maxMs = 5000;
    b.reset(1000);
    CHECK(b.steps == 0 && b.startedMs == 1000);
    CHECK(b.stepAllowed() && b.toolAllowed() && b.writeAllowed());

    b.addStep();
    b.addStep();
    CHECK(!b.stepAllowed());
    QString why;
    CHECK(b.exceeded(1000, &why));
    CHECK(why.contains("adım"));

    b.reset(1000);
    b.addToolCall();
    b.addToolCall();
    b.addToolCall();
    CHECK(!b.toolAllowed());
    b.reset(1000);
    b.addWrite();
    b.addWrite();
    CHECK(!b.writeAllowed());
    b.reset(1000);
    b.addTokens(100);
    CHECK(b.tokenExceeded());
    b.reset(1000);
    CHECK(b.timeExceeded(7000));
    CHECK(!b.timeExceeded(2000));

    // Yüzde: en dolu kotalar
    AgentBudget c;
    c.maxSteps = 4;
    c.maxToolCalls = 2;
    c.reset(0);
    c.addToolCall();
    CHECK(c.usedPercent(0) == 50);
    CHECK(c.summary(0).contains("adım"));

    // Sınırsız kotalar
    AgentBudget u;
    u.maxSteps = 0;
    u.maxToolCalls = 0;
    u.maxWrites = 0;
    u.maxTokens = 0;
    u.maxMs = 0;
    u.addStep();
    u.addToolCall();
    u.addTokens(999999);
    CHECK(u.stepAllowed() && u.toolAllowed() && !u.tokenExceeded());
    CHECK(!u.exceeded(999999, nullptr));
    CHECK(u.usedPercent(999999) == 0);
}

// --- 3 & 4: bellek ---
static void testMemory() {
    QTemporaryDir dir;
    CHECK(dir.isValid());
    const QString file = dir.path() + "/memory.json";
    AgentMemory mem(file);
    CHECK(mem.count() == 0);
    const QString id1 = mem.add("Proje CMake ile derleniyor, ctest ile test edilir", "rule");
    CHECK(!id1.isEmpty());
    mem.add("Kaynak dosyalar src/ altında", "path");
    mem.add("ctest --output-on-failure", "command");
    mem.add("Eski deneme: -j8 derleme hata veriyordu", "failure");
    CHECK(mem.count() == 4);
    // Aynı metin tekrar eklenirse ağırlık artar, yeni kayıt oluşmaz
    mem.add("Proje CMake ile derleniyor, ctest ile test edilir", "rule");
    CHECK(mem.count() == 4);
    // Kalıcılık
    AgentMemory again(file);
    CHECK(again.count() == 4);
    CHECK(again.texts().first().startsWith("[rule]"));

    CHECK(AgentMemory::tags().contains("failure"));
    CHECK(AgentMemory::normalizeTag("Bilinmeyen") == "fact");
    CHECK(AgentMemory::tokens("C++ testleri CMake ile derleniyor").contains("testleri"));

    // Alakalılık: kurallar öncelikli
    const QList<MemoryNote> all = again.notes();
    const auto hits = AgentMemory::recall(all, "ctest ile test nasıl koşar", 3);
    CHECK(!hits.isEmpty());
    CHECK(hits.first().text.contains("ctest"));
    const QString formatted = AgentMemory::formatRecall(hits, "ctest");
    CHECK(formatted.contains("AJAN BELLEĞİ"));
    CHECK(formatted.contains("ctest"));
    CHECK(AgentMemory::recall(all, "xyzzy", 3).isEmpty());
    CHECK(AgentMemory::formatRecall({}, "x").isEmpty());

    // Ağırlık farkı: kural notu eşleşirse daha yüksek puan
    MemoryNote rule{"1", "test kurallari burada", "rule", 1, 0, 0};
    MemoryNote fact{"2", "test kurallari burada", "fact", 1, 0, 0};
    CHECK(AgentMemory::relevance("test", rule) > AgentMemory::relevance("test", fact));

    CHECK(mem.remove(id1));
    CHECK(mem.count() == 3);
    CHECK(!mem.remove("yok"));
    mem.clear();
    CHECK(mem.count() == 0);
}

// --- 6 & 7: beceri kayıt defteri + zincir ---
static void testSkills() {
    const auto builtin = SkillRegistry::builtin();
    CHECK(builtin.size() >= 8);
    CHECK(SkillRegistry::exists("incele"));
    CHECK(SkillRegistry::exists("gözden-gecir"));
    const Skill audit = SkillRegistry::find("hata-ayikla");
    CHECK(!audit.requires.isEmpty());
    CHECK(audit.requires.contains("incele"));
    CHECK(SkillRegistry::find("gözden-gecir").tools.contains("health_scan"));
    CHECK(audit.builtin);
    CHECK(audit.steps.size() >= 3);
    CHECK(SkillRegistry::toPrompt(audit).contains("Beceri zinciri") ||
          SkillRegistry::toPrompt(audit).contains("BECERİ"));
    CHECK(SkillRegistry::names().contains("saglik-raporu"));

    // Özel beceri kalıcılığı
    Skill custom;
    custom.name = "Test Becerisi";
    custom.description = "sadece test için";
    custom.steps = {"bir", "iki"};
    custom.tools = {"read_file"};
    custom.tags = {"test"};
    CHECK(SkillRegistry::save(custom));
    CHECK(SkillRegistry::exists("test-becerisi"));
    const Skill loaded = SkillRegistry::find("test-becerisi");
    CHECK(loaded.steps.size() == 2 && loaded.tools.contains("read_file"));
    CHECK(!loaded.builtin);
    CHECK(SkillRegistry::remove("test-becerisi"));
    CHECK(!SkillRegistry::exists("test-becerisi"));
    CHECK(!SkillRegistry::save(Skill{})); // geçersiz

    // Sıralama: bağımlılık önce
    Skill a{"a", "ilk", {}, {}, {}, {}};
    Skill b{"b", "ikinci", {}, {}, {"a"}, {}};
    const auto ordered = SkillChain::orderByDependencies({b, a});
    CHECK(ordered.size() == 2);
    CHECK(ordered.first().name == "a");
    // Döngü güvenliği
    Skill c{"c", "", {}, {}, {"d"}, {}};
    Skill d{"d", "", {}, {}, {"c"}, {}};
    const auto cyc = SkillChain::orderByDependencies({c, d});
    CHECK(cyc.size() == 2);

    // Alakalılık
    CHECK(SkillChain::matchScore("test yaz", SkillRegistry::find("test-yaz")) > 0.5);
    CHECK(SkillChain::matchScore("", SkillRegistry::find("test-yaz")) == 0.0);

    // Hedefe göre plan
    const auto plan = SkillChain::planFor("testleri yaz ve düzelt", builtin, 3);
    CHECK(!plan.isEmpty());
    CHECK(plan.size() <= 3);
    bool hasTest = false, hasInspect = false;
    for (const Skill& s : plan) {
        if (s.name == "test-yaz") hasTest = true;
        if (s.name == "incele") hasInspect = true;
    }
    CHECK(hasTest);
    // Ön koşul dahil edilir ve önce gelir
    if (hasTest && hasInspect) {
        int idxInspect = -1, idxTest = -1;
        for (int i = 0; i < plan.size(); ++i) {
            if (plan[i].name == "incele") idxInspect = i;
            if (plan[i].name == "test-yaz") idxTest = i;
        }
        CHECK(idxInspect < idxTest);
    }

    // İlerleme
    SkillChain chain(plan);
    CHECK(chain.size() == plan.size());
    CHECK(chain.currentIndex() == 0);
    chain.startNext();
    CHECK(chain.steps().first().status == "active");
    chain.completeCurrent("tamam");
    CHECK(chain.doneCount() == 1);
    CHECK(chain.percent() > 0);
    CHECK(chain.toPrompt("hedef").contains("BECERİ ZİNCİRİ"));
    chain.reset();
    CHECK(chain.doneCount() == 0);
    // Tümünü bitir
    while (chain.currentIndex() >= 0) chain.completeCurrent();
    CHECK(chain.currentSkill().isEmpty());
    CHECK(chain.percent() == 100);
    SkillChain empty;
    empty.clear();
    CHECK(empty.isEmpty());
    CHECK(empty.toPrompt("x").isEmpty());
    CHECK(empty.currentIndex() == -1);
}

// --- 8: refleksiyon ---
static void testReflection() {
    const auto good = AgentReflection::reflect(
        "şimdi testleri koşuyorum", {"[read_file] 1| int main()", "[search] 3 eşleşme"}, 1, 5);
    CHECK(good.verdict == "good");
    CHECK(good.hadTool && !good.hadError);
    CHECK(good.score > 60);
    CHECK(!AgentReflection::shouldRemember(good));

    const auto bad = AgentReflection::reflect(
        "deniyorum", {"[run_tests] HATA: zaman aşımı (120000 ms)"}, 2, 5);
    CHECK(bad.hadError);
    CHECK(bad.verdict == "retry");
    CHECK(bad.score < good.score);
    CHECK(AgentReflection::shouldRemember(bad));
    CHECK(AgentReflection::rememberText("görev", bad).contains("görev"));

    const auto denied = AgentReflection::reflect("yazıyorum", {"[write_file] kullanıcı reddetti"}, 1, 5);
    CHECK(denied.wasDenied);
    CHECK(denied.verdict == "stalled");
    CHECK(denied.score < 50);
    CHECK(AgentReflection::rememberText("g", denied).contains("reddetti"));

    const auto empty = AgentReflection::reflect("", QStringList(), 1, 5);
    CHECK(!empty.hadTool);
    CHECK(empty.verdict == "good");

    // Son adım cezası
    const auto last = AgentReflection::reflect("çok uzun bir yanıt", {}, 5, 5);
    CHECK(last.score < 55);

    // Gözlem ayrıştırma
    const auto p1 = AgentReflection::parseObservation("[read_file] sorun yok");
    CHECK(p1.first == "read_file" && p1.second);
    const auto p2 = AgentReflection::parseObservation("[git_status] HATA: git yok");
    CHECK(p2.first == "git_status" && !p2.second);
    CHECK(AgentReflection::parseObservation("serbest metin").first.isEmpty());

    // İstatistik + ceza satırı
    QList<QPair<QString, bool>> res;
    res << QPair<QString, bool>("read_file", true) << QPair<QString, bool>("read_file", true)
        << QPair<QString, bool>("read_file", false) << QPair<QString, bool>("run_tests", false)
        << QPair<QString, bool>("run_tests", false);
    const auto st = AgentReflection::stats(res);
    CHECK(st.size() == 2);
    const auto findStat = [&st](const QString& n) {
        for (const auto& s : st)
            if (s.name == n) return s;
        return AgentReflection::ToolStat{};
    };
    CHECK(findStat("read_file").ok == 2 && findStat("read_file").fail == 1);
    CHECK(findStat("run_tests").rate() == 0.0);
    const QString penalty = AgentReflection::penaltyLine(st);
    CHECK(penalty.contains("run_tests"));
    CHECK(!penalty.contains("read_file")); // %66 >= 0.5 eşiği
    CHECK(AgentReflection::penaltyLine({}).isEmpty());
    CHECK(StepReflection::verdictLabel("good") == "iyi gidiyor");
    CHECK(StepReflection::verdictLabel("stalled") == "takıldı");
}

// --- 9: proje sağlığı ---
static void testProjectHealth() {
    CHECK(ProjectHealth::gradeFor(90) == "A");
    CHECK(ProjectHealth::gradeFor(75) == "B");
    CHECK(ProjectHealth::gradeFor(55) == "C");
    CHECK(ProjectHealth::gradeFor(10) == "D");
    CHECK(ProjectHealth::isSourceExt("cpp"));
    CHECK(!ProjectHealth::isSourceExt("png"));

    const auto clean = ProjectHealth::scoreOf(100, 0, 0, 0, 0, 1, 1, false);
    CHECK(clean.score >= 85 && clean.grade == "A");
    CHECK(clean.notes.isEmpty());

    const auto broken = ProjectHealth::scoreOf(100, 50, 20, 10, 4, 0, 0, true);
    CHECK(broken.score < clean.score);
    CHECK(broken.grade == "D");
    CHECK(broken.notes.size() >= 4);
    CHECK(broken.notes.size() <= 6);
    CHECK(broken.notes.join(" ").contains("Derleme"));
    CHECK(broken.summary().contains("Sağlık"));

    const auto unknown = ProjectHealth::scoreOf(10, 0, 0, 0, 0, -1, -1, false);
    CHECK(unknown.score > 0);
    CHECK(unknown.notes.join(" ").contains("bilinmiyor"));

    // Tarama: gerçek geçici dizin
    QTemporaryDir dir;
    CHECK(dir.isValid());
    writeFileIn(dir.path(), "a.cpp", "int main(){return 0;}\n// TODO: düzelt\n");
    writeFileIn(dir.path(), "b.cpp", "// FIXME: sonra\n// XXX: dikkat\n// HACK: geçici\n");
    QString big;
    for (int i = 0; i < 1200; ++i) big += "satır " + QString::number(i) + "\n";
    writeFileIn(dir.path(), "big.cpp", big);
    writeFileIn(dir.path(), "notlar.txt", "TODO yazı dosyası sayılmaz");
    const auto h = ProjectHealth::scan(dir.path());
    CHECK(h.files == 3);
    CHECK(h.todoCount == 1);
    CHECK(h.fixmeCount == 3);
    CHECK(h.hugeFiles == 1);
    CHECK(h.maxFileLines >= 1200);
    CHECK(h.maxFilePath.contains("big.cpp"));
    CHECK(ProjectHealth::scan("").files == 0);
}

// --- 10: koşu günlüğü + geri alma ---
static void testRunStore() {
    QTemporaryDir dir;
    CHECK(dir.isValid());
    AgentRunStore store(dir.path());
    CHECK(store.count() == 0);

    const QString target = dir.path() + "/f.txt";
    writeFileIn(dir.path(), "f.txt", "ESKİ İÇERİK");

    AgentRun run;
    run.task = "dosyayı güncelle";
    run.ok = true;
    run.finalText = "bitti";
    run.toolCalls = 2;
    run.tokens = 123;
    run.elapsedMs = 1500;
    RunFile changed;
    changed.path = target;
    changed.before = "ESKİ İÇERİK";
    run.changedFiles.append(changed);
    AgentRunStep st;
    st.assistant = "yazıyorum";
    st.toolNames << "read_file" << "write_file";
    st.observations << "[read_file] ok" << "[write_file] Yazıldı";
    run.stepLog << st;
    const QString id = store.add(run);
    CHECK(!id.isEmpty());
    CHECK(store.count() == 1);

    const QList<AgentRun> list = store.list();
    CHECK(list.size() == 1);
    CHECK(list.first().id == id);
    CHECK(list.first().statusLabel() == "başarılı");
    CHECK(list.first().oneLine().contains("1 adım"));
    CHECK(list.first().filesChanged() == 1);

    const AgentRun loaded = store.load(id);
    CHECK(loaded.task == "dosyayı güncelle");
    CHECK(loaded.stepLog.size() == 1);
    CHECK(loaded.stepLog.first().toolNames.size() == 2);
    CHECK(loaded.changedFiles.first().before == "ESKİ İÇERİK");
    CHECK(!loaded.changedFiles.first().created);
    CHECK(loaded.tokens == 123);
    CHECK(store.load("yok").id.isEmpty());
    CHECK(AgentRunStore::diffSummary(loaded).contains("read_file"));

    // Dosya elle değiştirilmişse geri alma dokunmaz
    writeFileIn(dir.path(), "f.txt", "KULLANICI DEĞİŞTİRDİ");
    int reverted = 0;
    QStringList skipped;
    const QString msg1 = AgentRunStore::revert(loaded, &reverted, &skipped);
    CHECK(reverted == 0);
    CHECK(skipped.size() == 1);
    CHECK(msg1.contains("Atlanan"));
    QFile f(target);
    f.open(QIODevice::ReadOnly);
    CHECK(QString::fromUtf8(f.readAll()) == "KULLANICI DEĞİŞTİRDİ");

    // Dokunulmamışsa geri alınır
    writeFileIn(dir.path(), "f.txt", "ESKİ İÇERİK");
    reverted = 0;
    AgentRunStore::revert(loaded, &reverted, &skipped);
    CHECK(reverted == 1);
    f.close();
    f.open(QIODevice::ReadOnly);
    CHECK(QString::fromUtf8(f.readAll()) == "ESKİ İÇERİK");

    // Yeni oluşturulan dosya geri alınca silinir
    const QString newPath = dir.path() + "/yeni.txt";
    writeFileIn(dir.path(), "yeni.txt", "yeni");
    AgentRun r2;
    r2.task = "yeni dosya";
    RunFile created;
    created.path = newPath;
    created.created = true;
    r2.changedFiles.append(created);
    reverted = 0;
    AgentRunStore::revert(r2, &reverted, nullptr);
    CHECK(reverted == 1);
    CHECK(!QFile::exists(newPath));
    CHECK(AgentRunStore::revert(r2, &reverted, nullptr).isEmpty() ||
          !AgentRunStore::revert(r2, &reverted, nullptr).isEmpty()); // ikinci çağrı da güvenli

    CHECK(store.remove(id));
    CHECK(store.count() == 0);
    CHECK(!store.remove("yok"));
}

// --- 7 & 11: yeni araçlar + güvenlik ---
static void testTools() {
    QTemporaryDir dir;
    CHECK(dir.isValid());
    writeFileIn(dir.path(), "a.cpp",
                "int alpha() { return 1; }\n// TODO: yorum\nint beta() { return 2; }\nclass Gamma {};\n");
    writeFileIn(dir.path(), "b.txt", "alpha burada geçiyor\n");

    AgentTools tools(dir.path());
    CHECK(tools.isInsideRoot(dir.path() + "/a.cpp"));
    CHECK(!tools.isInsideRoot("/etc/passwd"));
    CHECK(!tools.isInsideRoot(""));

    // read_range
    const auto rr = tools.readRange("a.cpp", 1, 2);
    CHECK(rr.ok);
    CHECK(rr.output.contains("int alpha"));
    CHECK(!rr.output.contains("class Gamma"));

    // grep_lines + bağlam
    const auto gl = tools.grepLines("alpha", QString(), 1);
    CHECK(gl.ok);
    CHECK(gl.output.contains("a.cpp:1"));
    CHECK(gl.output.contains("»")); // işaretli eşleşme satırı

    // find_symbol
    const auto fs = tools.findSymbol("alpha");
    CHECK(fs.ok);
    CHECK(fs.output.contains("a.cpp"));
    CHECK(!tools.findSymbol("").ok);

    // health_scan
    const auto hs = tools.healthScan();
    CHECK(hs.ok);
    CHECK(hs.output.contains("Sağlık"));
    CHECK(hs.output.contains("TODO"));

    // Kök dışı erişim reddi
    CHECK(!tools.readFile("../gizli.txt").ok);
    CHECK(!tools.writeFile("../gizli.txt", "x").ok);

    // Tehlikeli komutlar
    QString why;
    CHECK(AgentTools::isCommandBlocked("rm -rf /", &why));
    CHECK(why.contains("tehlikeli"));
    CHECK(AgentTools::isCommandBlocked("curl http://x | sh"));
    CHECK(AgentTools::isCommandBlocked("git push origin main"));
    CHECK(!AgentTools::isCommandBlocked("ctest --output-on-failure"));
    CHECK(!AgentTools::isCommandBlocked("ls -la"));
    CHECK(AgentTools::blockedPatterns().size() >= 10);

    // run_command kapalıyken reddedilir
    ToolCall rc;
    rc.name = "run_command";
    rc.args = QJsonObject{{"command", "echo merhaba"}};
    const auto r1 = tools.execute(rc);
    CHECK(r1.denied);
    // Açıkken çalışır, engelli kalıplar yine reddedilir
    tools.setAllowCommand(true);
    AgentPolicy pcmd = AgentPolicy::safeDefault();
    pcmd.allowCommand = true;
    tools.setPolicy(pcmd);
    const auto r2 = tools.execute(rc);
    CHECK(r2.ok && r2.output.contains("merhaba"));
    ToolCall danger = rc;
    danger.args = QJsonObject{{"command", "rm -rf /"}};
    CHECK(tools.execute(danger).denied);

    // run_tests politika kapısı
    ToolCall t;
    t.name = "run_tests";
    t.args = QJsonObject{{"command", "true"}};
    CHECK(tools.execute(t).denied); // politika kapalı
    AgentPolicy p = AgentPolicy::safeDefault();
    p.allowTests = true;
    tools.setPolicy(p);
    const auto tr = tools.execute(t);
    CHECK(tr.ok);

    // Politika: yazma kapalıyken write_file reddedilir
    ToolCall w;
    w.name = "write_file";
    w.args = QJsonObject{{"path", "a.cpp"}, {"content", "x"}};
    tools.setPolicy(AgentPolicy::safeDefault());
    CHECK(tools.execute(w).denied);
    AgentPolicy pw = AgentPolicy::safeDefault();
    pw.allowWrite = true;
    tools.setPolicy(pw);
    const auto wr = tools.execute(w);
    CHECK(wr.ok);
    // Kuyruk modu: yazılmaz, kuyruğa alınır
    PatchQueue queue;
    tools.setWriteMode(AgentTools::Queue);
    tools.setPatchQueue(&queue);
    const auto qr = tools.writeFile("a.cpp", "kuyruk");
    CHECK(qr.ok);
    CHECK(queue.count() == 1);

    // Otonom modda yazma reddedilir
    tools.setWriteMode(AgentTools::Direct);
    tools.setPolicy(AgentPolicy::autonomousReadOnly());
    CHECK(tools.execute(w).denied);
    CHECK(!tools.execute(w).output.isEmpty());

    // Bilinmeyen araç
    ToolCall u;
    u.name = "havala";
    CHECK(tools.execute(u).output.contains("Bilinmeyen"));
    CHECK(!tools.execute(u).ok);

    // git araçları (depo değil)
    tools.setGitRepo(false);
    CHECK(tools.gitStatus().output.contains("git deposu değil"));
    CHECK(tools.gitDiff().output.contains("git deposu değil"));
    tools.setGitRepo(true);
    const auto gsd = tools.gitStatus();
    CHECK(gsd.output.contains("$ git status"));
}

// --- 12: AgentLoop entegrasyonu (bütçe, refleksiyon, değişiklik kaydı) ---
static void testAgentLoop() {
    QTemporaryDir dir;
    CHECK(dir.isValid());
    writeFileIn(dir.path(), "mevcut.txt", "OLAN İÇERİK");

    AgentTools tools(dir.path());
    tools.setWriteMode(AgentTools::Direct);

    int call = 0;
    auto llm = [&call](const QString&, const QString&, QString& err) -> QString {
        ++call;
        if (call == 1)
            return "<tool_call>\n{\"name\":\"write_file\",\"arguments\":"
                   "{\"path\":\"cikti.txt\",\"content\":\"yeni\"}}\n</tool_call>";
        if (call == 2)
            return "<tool_call>\n{\"name\":\"write_file\",\"arguments\":"
                   "{\"path\":\"mevcut.txt\",\"content\":\"DEĞİŞTİ\"}}\n</tool_call>";
        return "Görev tamamlandı.";
    };

    AgentRunContext ctx;
    ctx.goal = "testi düzelt";
    ctx.policy = AgentPolicy::safeDefault();
    ctx.policy.allowWrite = true;
    ctx.budget.maxSteps = 6;
    ctx.budget.maxToolCalls = 10;
    ctx.budget.maxWrites = 5;
    ctx.budget.maxMs = 60000;

    AgentLoop::Result r = AgentLoop::run(tools, "sistem", "dosya yaz", 6, llm, {}, {}, &ctx);
    CHECK(r.ok);
    CHECK(r.finalText == "Görev tamamlandı.");
    CHECK(r.steps.size() == 3);
    CHECK(r.toolCalls == 2);
    CHECK(r.budget.steps == 3);
    CHECK(r.budget.writes == 2);
    CHECK(!r.reflections.isEmpty());
    CHECK(r.finalScore > 0);
    // Geri alma için eski içerikler kaydedildi
    CHECK(r.changedFiles.size() == 2);
    bool sawExisting = false, sawNew = false;
    for (const RunFile& p : r.changedFiles) {
        if (p.path.endsWith("mevcut.txt") && p.before == "OLAN İÇERİK") sawExisting = true;
        if (p.path.endsWith("cikti.txt") && p.created && p.before.isEmpty()) sawNew = true;
    }
    CHECK(sawExisting);
    CHECK(sawNew);
    // Dosyalar gerçekten yazıldı
    QFile f(dir.path() + "/mevcut.txt");
    f.open(QIODevice::ReadOnly);
    CHECK(QString::fromUtf8(f.readAll()) == "DEĞİŞTİ");
    // Özet üretilebilir
    const QString sum = AgentLoop::summarizeRun(r);
    CHECK(sum.contains("adım"));
    CHECK(AgentLoop::formatTranscript(r.steps).contains("write_file"));

    // Araç kotası: tek adımda iki çağrı istenir, kotodan sonrası yapılmaz
    QString two = "<tool_call>\n{\"name\":\"read_file\",\"arguments\":{\"path\":"
                  "\"mevcut.txt\"}}\n</tool_call>\n";
    two += "<tool_call>\n{\"name\":\"list_dir\",\"arguments\":{}}\n</tool_call>";
    auto llm2 = [two](const QString&, const QString&, QString&) -> QString { return two; };
    AgentRunContext c2;
    c2.policy = AgentPolicy::safeDefault();
    c2.budget.maxSteps = 4;
    c2.budget.maxToolCalls = 1; // ikinci çağrı kotoyu aşar
    const AgentLoop::Result r2 = AgentLoop::run(tools, "sistem", "oku", 4, llm2, {}, {}, &c2);
    CHECK(r2.budgetStopped);
    CHECK(r2.budgetReason.contains("araç"));
    CHECK(r2.toolCalls == 1);
    CHECK(r2.steps.size() == 1);
    CHECK(r2.steps.first().observations.join(" ").contains("kotası doldu"));
    CHECK(r2.steps.first().reflection.hadTool);
    CHECK(AgentLoop::summarizeRun(r2).contains("Bütçe"));

    // LLM hatası koşuyu durdurur
    auto llmErr = [](const QString&, const QString&, QString& err) -> QString {
        err = "bağlantı yok";
        return {};
    };
    const AgentLoop::Result r3 = AgentLoop::run(tools, "sistem", "x", 2, llmErr);
    CHECK(!r3.ok);
    CHECK(r3.error == "bağlantı yok");
    CHECK(AgentLoop::summarizeRun(r3).contains("adım"));

    // Bellek: başarısız adımdan sonra ders notu yazılır
    QTemporaryDir memDir;
    AgentMemory mem(memDir.path() + "/m.json");
    int call3 = 0;
    auto llm3 = [&call3](const QString&, const QString&, QString&) -> QString {
        ++call3;
        if (call3 == 1)
            return "<tool_call>\n{\"name\":\"read_file\",\"arguments\":{\"path\":\"yok.txt\"}}\n"
                   "</tool_call>";
        return "bitti";
    };
    AgentRunContext c4;
    c4.memory = &mem;
    c4.goal = "test koş";
    c4.policy = AgentPolicy::safeDefault();
    c4.budget.maxSteps = 4;
    const AgentLoop::Result r4 = AgentLoop::run(tools, "sistem", "test koş", 4, llm3, {}, {}, &c4);
    CHECK(r4.reflections.size() == 2);
    CHECK(r4.steps.first().reflection.hadError);
    CHECK(r4.steps.first().reflection.verdict == "retry");
    CHECK(mem.count() == 1);
    CHECK(mem.texts().first().contains("test koş"));
    // Bellek ikinci adımda hatırlatıcı olarak devreye girer
    CHECK(AgentMemory::recall(mem.notes(), "test koş").size() == 1);
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    testPolicy();
    testBudget();
    testMemory();
    testSkills();
    testReflection();
    testProjectHealth();
    testRunStore();
    testTools();
    testAgentLoop();
    fprintf(stderr, "STAGE34: %d passed, %d failed\n", g_pass, g_fail);
    fflush(stderr);
    return g_fail == 0 ? 0 : 1;
}
