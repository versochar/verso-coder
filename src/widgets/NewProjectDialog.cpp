#include "NewProjectDialog.h"
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

NewProjectDialog::NewProjectDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("Yeni Proje");
    setMinimumWidth(420);
    auto* lay = new QVBoxLayout(this);
    auto* form = new QFormLayout();
    m_tpl = new QComboBox(this);
    m_tpl->addItems({"C++ (CMake)", "Python (paket)", "Qt Widget (CMake)"});
    m_name = new QLineEdit("benim-projem", this);
    m_dir = new QLineEdit(QDir::homePath(), this);
    auto* dirRow = new QHBoxLayout();
    dirRow->addWidget(m_dir, 1);
    auto* bBrowse = new QPushButton("...", this);
    bBrowse->setFixedWidth(36);
    dirRow->addWidget(bBrowse);
    connect(bBrowse, &QPushButton::clicked, this, [this]() {
        const QString d = QFileDialog::getExistingDirectory(this, "Konum seç", m_dir->text());
        if (!d.isEmpty()) m_dir->setText(d);
    });
    form->addRow("Şablon:", m_tpl);
    form->addRow("Ad:", m_name);
    form->addRow("Konum:", dirRow);
    lay->addLayout(form);
    auto* box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                     this);
    box->button(QDialogButtonBox::Ok)->setText("Oluştur");
    connect(box, &QDialogButtonBox::accepted, this, &NewProjectDialog::create);
    connect(box, &QDialogButtonBox::rejected, this, &NewProjectDialog::reject);
    lay->addWidget(box);
}

QMap<QString, QString> NewProjectDialog::templateFiles(const QString& tpl,
                                                       const QString& name) {
    QMap<QString, QString> out;
    if (tpl.startsWith("Python")) {
        out["pyproject.toml"] =
            "[project]\nname = \"" + name + "\"\nversion = \"0.1.0\"\n";
        out["src/" + name + "/__init__.py"] = "";
        out["src/" + name + "/main.py"] =
            "def main():\n    print(\"Merhaba, " + name + "!\")\n\n\n"
            "if __name__ == \"__main__\":\n    main()\n";
        out["tests/test_main.py"] = "def test_ornek():\n    assert True\n";
        out[".gitignore"] = "__pycache__/\n.venv/\n";
    } else if (tpl.startsWith("Qt")) {
        out["CMakeLists.txt"] =
            "cmake_minimum_required(VERSION 3.16)\nproject(" + name +
            ")\nset(CMAKE_CXX_STANDARD 17)\nfind_package(Qt6 REQUIRED COMPONENTS Widgets)\n"
            "add_executable(" +
            name +
            " main.cpp mainwindow.cpp)\ntarget_link_libraries(" + name +
            " PRIVATE Qt6::Widgets)\n";
        out["main.cpp"] =
            "#include <QApplication>\n#include \"mainwindow.h\"\n\nint main(int argc, char** argv) {\n"
            "    QApplication app(argc, argv);\n    MainWindow w;\n    w.show();\n    return app.exec();\n}\n";
        out["mainwindow.h"] =
            "#pragma once\n#include <QMainWindow>\n\nclass MainWindow : public QMainWindow {\n"
            "    Q_OBJECT\npublic:\n    explicit MainWindow(QWidget* parent = nullptr);\n};\n";
        out["mainwindow.cpp"] =
            "#include \"mainwindow.h\"\n\nMainWindow::MainWindow(QWidget* parent)\n"
            "    : QMainWindow(parent) {\n    setWindowTitle(\"" +
            name + "\");\n}\n";
        out[".gitignore"] = "build/\n";
    } else {
        out["CMakeLists.txt"] =
            "cmake_minimum_required(VERSION 3.16)\nproject(" + name +
            ")\nset(CMAKE_CXX_STANDARD 17)\nadd_executable(" + name + " src/main.cpp)\n";
        out["src/main.cpp"] =
            "#include <iostream>\n\nint main() {\n    std::cout << \"Merhaba, " + name +
            "!\\n\";\n    return 0;\n}\n";
        out[".gitignore"] = "build/\n";
    }
    return out;
}

void NewProjectDialog::create() {
    const QString name = m_name->text().trimmed();
    const QString dir = m_dir->text().trimmed();
    if (name.isEmpty() || dir.isEmpty()) return;
    const QString root = QDir(dir).absoluteFilePath(name);
    if (QFileInfo::exists(root)) {
        QMessageBox::warning(this, "Yeni Proje", "Hedef zaten var: " + root);
        return;
    }
    QDir().mkpath(root);
    const auto files = templateFiles(m_tpl->currentText(), name);
    for (auto it = files.constBegin(); it != files.constEnd(); ++it) {
        const QString p = QDir(root).absoluteFilePath(it.key());
        QDir().mkpath(QFileInfo(p).absolutePath());
        QFile f(p);
        if (f.open(QIODevice::WriteOnly | QIODevice::Truncate))
            f.write(it.value().toUtf8());
    }
    m_created = root;
    accept();
}
