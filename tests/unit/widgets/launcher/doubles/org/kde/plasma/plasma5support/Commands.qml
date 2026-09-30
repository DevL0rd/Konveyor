pragma Singleton
import QtQuick

QtObject {
    property var log: []
    property var responses: []
    property var handled: []

    function record(command) {
        log = log.concat([command])
        for (const response of responses) {
            if (new RegExp(response.match).test(command))
                return { "exit code": response.exitCode || 0, stdout: response.stdout || "", stderr: response.stderr || "" }
        }
        return null
    }
    function finish(command) {
        handled = handled.concat([command])
    }
}
