#pragma once
#include <QString>
#include <QStringList>

// Stage 38: bellek sızıntısı ölçümü. Yinelenen işlemlerde RSS artışını
// izler; "her turda biraz büyüyor" kalıbı gerçek sızıntı işaretidir.
class LeakWatch {
public:
    explicit LeakWatch(const QString& label = QString(), int iterations = 0);

    // Başlangıç ölçümü (GC/önbellek temizliği sonrası)
    void reset();
    // Bir tur tamamlandı (iterations sayacı artar)
    void tick();
    // Anlık RSS (MB). Linux /proc, diğer sistemlerde 0 döner.
    static double rssMb();
    // Ölçüm sonucu
    struct Result {
        bool available = false;
        int iterations = 0;
        double startMb = 0.0;
        double peakMb = 0.0;
        double endMb = 0.0;
        double growthMb = 0.0;     // tur başına ortalama artış
        double growthPerIterKb = 0.0;
        bool leaky = false;        // tur başına eşik aşımı
        QString verdict;
    };
    Result result() const;

    // Tur başına kabul edilebilir artış eşiği (KB). Varsayılan 64 KB:
    // dosya önbelleği/LSP önbelleği gibi meşru büyümeler bunu aşmaz,
    // gerçek sızıntılar açar.
    static double growthThresholdKb();
    static void setGrowthThresholdKb(double kb);

private:
    QString m_label;
    int m_iterations = 0;
    int m_target = 0;
    double m_start = 0.0;
    double m_peak = 0.0;
    double m_end = 0.0;
    double m_worst = 0.0; // en büyük tur artışı (KB)
    double m_prev = 0.0;
};
