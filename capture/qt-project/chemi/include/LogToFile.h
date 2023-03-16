#ifndef LOGTOFILE_H
#define LOGTOFILE_H

#include <QStandardPaths>

#include "spdlog/spdlog.h"
#include "spdlog/sinks/rotating_file_sink.h"

#include "WzGlobalConst.h"

void LogToFileInit();

#endif // LOGTOFILE_H
