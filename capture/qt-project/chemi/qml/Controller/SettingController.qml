import QtQuick 2.12

Item {
    id: root
    signal response(string response)

    Connections {
        target: window
        onDebugCommand: {
            if (!cmd.startsWith("setting"))
                return
            func.parseCommand(cmd)
        }
    }

    QtObject {
        id: func
        function parseCommand(cmd) {
            var params = cmd.split(",", 3)
            if (params.length < 2)
                return false

            var optionName = params[1]

            // get
            if (params.length === 2) {
                var response = "setting," + optionName + ","
                if (dbService.existsStrOption(optionName)) {
                    var value = dbService.readStrOption(optionName, "<nothing>")
                    response += value
                } else {
                    response += "<nothing>"
                }
                root.response(response)
            }
            // set
            else {
                var newValue = params[2]
                dbService.saveStrOption(optionName, newValue)
            }
        }
    }
}
