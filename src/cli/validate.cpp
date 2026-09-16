#include "validate.h"

#include "config/loader.h"

namespace Konveyor::Cli
{

int runValidate(const QString &path, QTextStream &out, QTextStream &err)
{
    const auto result = Config::loadFile(path);
    if (!result) {
        err << result.error().toString() << "\n";
        err.flush();
        return 1;
    }
    for (const QString &warning : result->warnings) {
        err << "warning: " << warning << "\n";
    }
    err.flush();
    out << "Config is valid: " << path << "\n";
    out.flush();
    return 0;
}

}
