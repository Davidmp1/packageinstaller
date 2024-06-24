#include "cmd.hpp"

#include <QEventLoop>
#include <fmt/core.h>
#include <fmt/ranges.h>
#include <spdlog/spdlog.h>

Cmd::Cmd(QObject* parent)
  : QProcess(parent) {
    connect(this, &Cmd::readyReadStandardOutput, [this]() { emit outputAvailable(readAllStandardOutput()); });
    connect(this, &Cmd::readyReadStandardError, [this]() { emit errorAvailable(readAllStandardError()); });
    connect(this, &Cmd::outputAvailable, [this](const QString& out) { out_buffer += out; });
    connect(this, &Cmd::errorAvailable, [this](const QString& out) { out_buffer += out; });
}

void Cmd::halt() {
    if (this->state() == QProcess::NotRunning) {
        return;
    }
    terminate();
    waitForFinished(5000);
    kill();
    waitForFinished(1000);
}

bool Cmd::run(const QString& cmd, bool quiet) {
    out_buffer.clear();
    QString output;
    return run(cmd, output, quiet);
}

// util function for getting bash command output
QString Cmd::getCmdOut(const QString& cmd, bool quiet) {
    out_buffer.clear();
    QString output;
    run(cmd, output, quiet);
    return output;
}

bool Cmd::run(const QString& cmd, QString& output, bool quiet) {
    out_buffer.clear();
    connect(this, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &Cmd::finished, Qt::UniqueConnection);
    if (this->state() != QProcess::NotRunning) {
        std::vector<std::string> args_vec(static_cast<std::size_t>(this->arguments().size()));
        for (std::size_t i = 0; i < args_vec.size(); ++i) {
            args_vec[i] = this->arguments()[static_cast<std::int32_t>(i)].toStdString();
        }
        spdlog::debug("Process already running:{},{}", this->program().toStdString(), fmt::join(args_vec, ", "));
        return false;
    }
    if (!quiet) {
        spdlog::debug("{}", cmd.toStdString().data());
    }

    QEventLoop loop;
    connect(this, &Cmd::finished, &loop, &QEventLoop::quit);
    start("/bin/bash", {"-c", cmd});
    loop.exec();
    output = out_buffer.trimmed();
    return (exitStatus() == QProcess::NormalExit && exitCode() == 0);
}
