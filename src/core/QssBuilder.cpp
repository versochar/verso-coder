#include "QssBuilder.h"
#include "AccentColor.h"
#include "UiMetrics.h"

QColor QssBuilder::effectiveAccent(const Input& in) {
    return in.accentOverride.isValid() ? in.accentOverride : in.tokens.accent;
}

QString QssBuilder::build(const Input& in) {
    const ThemeTokens& t = in.tokens;
    const QColor accent = effectiveAccent(in);
    const QColor accentHover = AccentColor::hover(accent, t.dark);
    const QColor accentPressed = AccentColor::pressed(accent, t.dark);
    const QColor accentSoft = AccentColor::soft(accent, t.bg, 0.22);
    const QColor accentSofter = AccentColor::soft(accent, t.bg, 0.12);
    const QColor hoverBg = t.dark ? ThemeTokens::mix(t.surfaceAlt, t.textStrong, 0.05)
                                  : ThemeTokens::mix(t.surfaceAlt, t.text, 0.04);
    const QColor borderSoft = t.dark ? ThemeTokens::mix(t.border, t.bg, 0.35)
                                     : ThemeTokens::mix(t.border, t.bg, 0.25);

    const int r = int(6 * in.scale + 0.5);
    const int rLg = int(10 * in.scale + 0.5);
    const int sp = int(6 * in.scale + 0.5);

    const QString font = QString("font-family: \"%1\"; font-size: %2px;")
                             .arg(in.typo.uiFamily.isEmpty() ? "Segoe UI" : in.typo.uiFamily)
                             .arg(qMax(9, in.typo.uiSize));

    QString q;
    q += "/* Verso Coder — %1 (token tabanlı, otomatik üretim) */\n\n";

    // ---- temel ----
    q += QString("QWidget { background: %1; color: %2; %3 }\n")
             .arg(t.bg.name(), t.text.name(), font);
    q += QString("QMainWindow::separator { background: %1; width: 1px; height: 1px; }\n").arg(t.border.name());
    q += QString("QSplitter::handle { background: %1; }\n"
                 "QSplitter::handle:hover { background: %2; }\n")
             .arg(borderSoft.name(), accentSoft.name());

    // ---- buton ----
    q += QString(
             "QPushButton { background: %1; color: %2; border: 1px solid %3;\n"
             "  border-radius: %4px; padding: %5px %6px; }\n"
             "QPushButton:hover { background: %7; border-color: %8; }\n"
             "QPushButton:pressed { background: %9; }\n"
             "QPushButton:checked { background: %10; color: %11; border-color: %12; }\n"
             "QPushButton:disabled { color: %13; background: %14; }\n"
             "QPushButton:focus { border: %12; }\n")
             .arg(t.surfaceAlt.name(), t.text.name(), t.border.name())
             .arg(r).arg(sp).arg(sp * 2)
             .arg(hoverBg.name(), ThemeTokens::mix(t.border, accent, 0.4).name())
             .arg(t.dark ? ThemeTokens::mix(t.surfaceAlt, t.textStrong, 0.10).name()
                         : ThemeTokens::mix(t.surfaceAlt, t.text, 0.08).name())
             .arg(accentSoft.name(), ThemeTokens::mix(accent, t.textStrong, t.dark ? 0.6 : 0.0).name(),
                  accent.name())
             .arg(t.textDim.name(), t.surface.name());

    // ---- girişler ----
    const QString inputs = "QLineEdit, QPlainTextEdit, QTextEdit, QComboBox, QSpinBox, "
                           "QDoubleSpinBox, QKeySequenceEdit";
    q += QString(
             "%1 { background: %2; color: %3; border: 1px solid %4;\n"
             "  border-radius: %5px; padding: %6px %7px; selection-background-color: %8; }\n"
             "%1:focus { border: 1px solid %9; }\n"
             "%1:disabled { color: %10; }\n"
             "QComboBox::drop-down { border: none; width: 22px; }\n"
             "QComboBox QAbstractItemView { background: %2; color: %3; border: 1px solid %4;\n"
             "  border-radius: %5px; selection-background-color: %11; outline: none; }\n")
             .arg(inputs, t.dark ? ThemeTokens::mix(t.bg, t.textStrong, 0.04).name() : t.surface.name(),
                  t.text.name(), t.border.name())
             .arg(QString::number(r), QString::number(sp), QString::number(sp))
             .arg(t.selection.name(), accent.name(), t.textDim.name(), accentSoft.name());

    // ---- menüler ----
    q += QString(
             "QMenuBar { background: %1; color: %2; border-bottom: 1px solid %3; }\n"
             "QMenuBar::item:selected { background: %4; border-radius: %5px; }\n"
             "QMenu { background: %6; color: %2; border: 1px solid %3; border-radius: %5px; padding: %7px; }\n"
             "QMenu::item { padding: %8px %9px; border-radius: %5px; }\n"
             "QMenu::item:selected { background: %10; }\n"
             "QMenu::separator { height: 1px; background: %3; margin: %7px; }\n")
             .arg(t.surface.name(), t.text.name(), t.border.name(), accentSofter.name(),
                   QString::number(rLg), t.surface.name(), QString::number(sp),
                   QString::number(int(4 * in.scale + 0.5)),
                   QString::number(int(12 * in.scale + 0.5)), accentSoft.name());

    // ---- sekmeler (modern: aktif alt çizgi accent) ----
    q += QString(
             "QTabWidget::pane { border: none; border-top: 1px solid %1; top: -1px; }\n"
             "QTabWidget::tab-bar { background: %2; }\n"
             "QTabBar { background: %2; }\n"
             "QTabBar::tab { background: transparent; color: %3;\n"
             "  padding: %4px %5px; margin-right: 2px;\n"
             "  border-top-left-radius: %6px; border-top-right-radius: %6px; }\n"
             "QTabBar::tab:hover { color: %7; background: %8; }\n"
             "QTabBar::tab:selected { color: %7; background: %9;\n"
             "  border-bottom: 2px solid %10; }\n"
             "QTabBar::close-button { image: none; subcontrol-position: right; }\n")
             .arg(t.border.name(), t.surface.name(), t.textDim.name())
             .arg(QString::number(int(7 * in.scale + 0.5)),
                   QString::number(int(12 * in.scale + 0.5)), QString::number(r))
             .arg(t.text.name(), ThemeTokens::withAlphaF(t.textStrong, 0.04).name(), t.bg.name(),
                  accent.name());

    // ---- ağaçlar / listeler / tablolar ----
    q += QString(
             "QTreeView, QTreeWidget, QListView, QListWidget { background: %1; color: %2;\n"
             "  border: none; outline: none; }\n"
             "QTreeView::item, QTreeWidget::item, QListView::item, QListWidget::item {\n"
             "  padding: %3px; border-radius: %4px; }\n"
             "QTreeView::item:hover, QTreeWidget::item:hover, QListView::item:hover, QListWidget::item:hover {\n"
             "  background: %5; }\n"
             "QTreeView::item:selected, QTreeWidget::item:selected, QListView::item:selected, QListWidget::item:selected {\n"
             "  background: %6; color: %7; }\n"
             "QHeaderView::section { background: %1; color: %8; border: none;\n"
             "  border-bottom: 1px solid %9; padding: %3px; }\n"
             "QTableWidget { gridline-color: %9; }\n")
             .arg(t.bg.name(), t.text.name(), QString::number(int(3 * in.scale + 0.5)),
                   QString::number(r), hoverBg.name(),
                  accentSoft.name(), t.textStrong.name(), t.textDim.name(), borderSoft.name());

    // ---- durum çubuğu + çipler ----
    q += QString(
             "QStatusBar { background: %1; color: %2; border-top: 1px solid %3; }\n"
             "QStatusBar::item { border: none; }\n"
             "QStatusBar QLabel { color: %2; padding: 0 3px; }\n"
             "QLabel#chip { background: %4; color: %5; border-radius: %6px;\n"
             "  padding: 1px 8px; margin: 0 1px; font-size: %7px; }\n"
             "QLabel#chipWarn { background: %8; color: %9; border-radius: %6px; padding: 1px 8px; }\n"
             "QLabel#chipErr  { background: %10; color: %11; border-radius: %6px; padding: 1px 8px; }\n"
             "QLabel#chipOk   { background: %12; color: %13; border-radius: %6px; padding: 1px 8px; }\n"
             "QLabel#chipAccent { background: %14; color: %15; border-radius: %6px; padding: 1px 8px; }\n")
             .arg(t.surface.name(), t.textDim.name(), t.border.name())
             .arg(t.surfaceAlt.name(), t.text.name(),
                   QString::number(int(8 * in.scale + 0.5)),
                   QString::number(qMax(9, in.typo.uiSize - 2)))
             .arg(AccentColor::soft(t.warning, t.surface, 0.25).name(), t.warning.name())
             .arg(AccentColor::soft(t.error, t.surface, 0.25).name(), t.error.name())
             .arg(AccentColor::soft(t.success, t.surface, 0.25).name(), t.success.name())
             .arg(accentSoft.name(),
                  t.dark ? ThemeTokens::mix(accent, t.textStrong, 0.7).name() : accent.name());

    // ---- kaydırma çubukları ----
    q += QString(
             "QScrollBar:vertical { background: transparent; width: 10px; margin: 0; }\n"
             "QScrollBar::handle:vertical { background: %1; border-radius: 5px; min-height: 24px; }\n"
             "QScrollBar::handle:vertical:hover { background: %2; }\n"
             "QScrollBar:horizontal { background: transparent; height: 10px; margin: 0; }\n"
             "QScrollBar::handle:horizontal { background: %1; border-radius: 5px; min-width: 24px; }\n"
             "QScrollBar::handle:horizontal:hover { background: %2; }\n"
             "QScrollBar::add-line, QScrollBar::sub-line { width: 0; height: 0; }\n"
             "QScrollBar::add-page, QScrollBar::sub-page { background: transparent; }\n")
             .arg(ThemeTokens::withAlphaF(t.scrollbar, t.dark ? 0.75 : 1.0).name(),
                  ThemeTokens::mix(t.scrollbar, accent, 0.35).name());

    // ---- kalıtım/görsel parçalar ----
    q += QString(
             "QToolTip { background: %1; color: %2; border: 1px solid %3; border-radius: %4px; padding: 3px 8px; }\n"
             "QCheckBox, QRadioButton { spacing: 6px; }\n"
             "QCheckBox::indicator, QRadioButton::indicator { width: 15px; height: 15px; }\n"
             "QGroupBox { border: 1px solid %3; border-radius: %4px; margin-top: 8px; padding-top: 4px; }\n"
             "QGroupBox::title { subcontrol-origin: margin; left: %5px; padding: 0 3px; color: %6; }\n"
             "QProgressBar { background: %7; border: 1px solid %3; border-radius: %4px; text-align: center; height: 14px; }\n"
             "QProgressBar::chunk { background: %8; border-radius: %4px; }\n"
             "QSlider::groove:horizontal { height: 4px; background: %7; border-radius: 2px; }\n"
             "QSlider::handle:horizontal { width: 13px; height: 13px; margin: -5px 0;\n"
             "  border-radius: 7px; background: %8; }\n"
             "QDockWidget::title { background: %9; color: %6; padding: 4px 8px;\n"
             "  border-bottom: 1px solid %3; }\n")
             .arg(t.surface.name(), t.textStrong.name(), t.border.name(), QString::number(r),
                  QString::number(int(8 * in.scale + 0.5)), t.textDim.name(),
                  t.surfaceAlt.name(), accent.name(), t.surface.name());

    // ---- QTabWidget dok sekmeleri (alt panel: documentMode düz görünüm) ----
    q += QString("QTabWidget QTabBar::tab { font-size: %1px; }\n")
             .arg(qMax(9, in.typo.uiSize - 1));

    // ---- Stage 10: toast bildirimleri ----
    q += QString(
             "QFrame#toast { background: %1; border: 1px solid %2; border-radius: %3px; }\n"
             "QFrame#toast QLabel { color: %4; font-size: %5px; }\n"
             "QFrame#toast[severity=\"info\"]    { border-left: 3px solid %6; }\n"
             "QFrame#toast[severity=\"success\"] { border-left: 3px solid %7; }\n"
             "QFrame#toast[severity=\"warning\"] { border-left: 3px solid %8; }\n"
             "QFrame#toast[severity=\"error\"]   { border-left: 3px solid %9; }\n")
             .arg(t.surface.name(), t.border.name(), QString::number(rLg),
                  t.textStrong.name(), QString::number(qMax(9, in.typo.uiSize)),
                  accent.name(), t.success.name(), t.warning.name(), t.error.name());

    // ---- Stage 10: karşılama ekranı + boş durumlar + palet sayacı ----
    q += QString(
             "QLabel#welcomeTitle { font-size: %1px; font-weight: bold; color: %2; }\n"
             "QLabel#welcomeSub   { color: %3; }\n"
             "QLabel#welcomeSection { font-weight: bold; color: %4; font-size: %5px; }\n"
             "QFrame#welcomeCard { background: %6; border: 1px solid %7; border-radius: %8px; }\n"
             "QPushButton#welcomeAction { background: transparent; border: none; border-radius: %9px; text-align: left; }\n"
             "QPushButton#welcomeAction:hover { background: %10; }\n"
             "QLabel#welcomeActionTitle { font-weight: bold; color: %2; }\n"
             "QLabel#welcomeActionSub, QLabel#welcomeKeySub { color: %3; }\n"
             "QLabel#welcomeKey { color: %11; background: %12; border-radius: 4px; padding: 2px 8px; }\n"
             "QLabel#paletteCount { color: %3; font-size: %5px; padding: 2px; }\n"
             "QLabel#emptyTitle { font-size: %13px; font-weight: bold; color: %2; }\n"
             "QLabel#emptySub { color: %3; }\n"
             "QPushButton#emptyAction { color: %11; font-weight: bold; }\n")
             .arg(QString::number(qMax(18, in.typo.uiSize + 10)), t.textStrong.name(),
                  t.textDim.name(), t.text.name(), QString::number(qMax(9, in.typo.uiSize - 1)),
                  t.surface.name(), t.border.name(), QString::number(rLg),
                  QString::number(r), ThemeTokens::withAlphaF(t.text, 0.06).name(),
                  t.dark ? ThemeTokens::mix(t.accent, t.textStrong, 0.6).name() : t.accent.name(),
                  t.surfaceAlt.name(),
                  QString::number(qMax(14, in.typo.uiSize + 4)));

    // ---- Stage 11: ince kaydırma çubukları + bulma çubuğu + sticky ----
    q += QString(
             "QScrollBar:vertical { background: transparent; width: %1px; margin: 0; }\n"
             "QScrollBar::handle:vertical { background: %2; border-radius: %3px; min-height: 24px; }\n"
             "QScrollBar::handle:vertical:hover { background: %4; }\n"
             "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }\n"
             "QScrollBar:horizontal { background: transparent; height: %1px; margin: 0; }\n"
             "QScrollBar::handle:horizontal { background: %2; border-radius: %3px; min-width: 24px; }\n"
             "QScrollBar::handle:horizontal:hover { background: %4; }\n"
             "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }\n"
             "QWidget#findBar { background: %5; border: 1px solid %6; border-radius: %7px; }\n"
             "QWidget#findBar QLabel#findCount { color: %8; }\n"
             "QLineEdit#findEdit { border: 1px solid %9; border-radius: %7px; padding: 3px 6px; }\n"
             "QPushButton#stickyBar { background: %5; border: none; border-bottom: 1px solid %6;"
             " text-align: left; padding-left: 8px; color: %8; }\n"
             "QPushButton#stickyBar:hover { color: %10; }\n")
             .arg(QString::number(qMax(6, int(10 * in.scale + 0.5))),
                  ThemeTokens::withAlphaF(t.textDim, 0.45).name(),
                  QString::number(qMax(2, r / 2)),
                  ThemeTokens::withAlphaF(t.textDim, 0.75).name(),
                  t.surface.name(), t.border.name(), QString::number(r),
                  t.textDim.name(), t.accent.name(), t.textStrong.name());

    // ---- Stage 12: özel başlık çubuğu ----
    q += QString(
             "QWidget#titleBar { background: %1; border-bottom: 1px solid %2; }\n"
             "QWidget#titleBar QLabel#titleLabel { color: %3; font-size: %4px; }\n"
             "QPushButton#titleBtn { background: transparent; border: none; color: %3;"
             " border-radius: 4px; font-size: 12px; }\n"
             "QPushButton#titleBtn:hover { background: %5; color: %6; }\n"
             "QPushButton#titleCloseBtn { background: transparent; border: none; color: %3;"
             " border-radius: 4px; font-size: 12px; }\n"
             "QPushButton#titleCloseBtn:hover { background: %7; color: #ffffff; }\n")
             .arg(t.surface.name(), t.border.name(), t.textDim.name(),
                  QString::number(qMax(9, in.typo.uiSize - 1)),
                  ThemeTokens::withAlphaF(t.text, 0.10).name(), t.textStrong.name(),
                  t.error.name());

    return q.arg(t.name);
}
