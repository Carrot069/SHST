import QtQuick 2.12
import QtQuick.Controls 2.12

RadioButton {
    id: control
    text: qsTr("RadioButton")
    checked: true
    height: 30
    opacity: enabled ? 1 : 0.5

    Behavior on opacity{NumberAnimation{duration: 200}}

    hoverEnabled: true
    onHoveredChanged: updateState()
    onDownChanged: updateState()
    onCheckedChanged: updateState()
    onEnabledChanged: updateState()

    property color borderColorUnchecked: "#a0a0a0"
    property color borderColorUncheckedHover: "#a0a0a0"
    property color borderColorUncheckedDown: "#a0a0a0"
    property color borderColorChecked: "#009be3"
    property color borderColorCheckedHover: "#009be3"
    property color borderColorCheckedDown: "#a0a0a0"
    property color borderColorCheckedDisabled: "#a0a0a0"

    property int indicatorSize: 20

    /*
      unchecked
      uncheckedHover
      uncheckedDown
      uncheckedDisabled
      checked
      checkedHover
      checkedDown
      checkedDisabled
    */

    function updateState() {
        if (checked) {
            if (enabled) {
                if (down) {
                    control.state = "checkedDown"
                } else if (hovered) {
                    control.state = "checkedHover"
                } else {
                    control.state = "checked"
                }
            } else {
                control.state = "checkedDisabled"
            }
        } else {
            if (enabled) {
                if (down) {
                    control.state = "uncheckedDown"
                } else if (hovered) {
                    control.state = "uncheckedHover"
                } else {
                    control.state = "unchecked"
                }
            } else {
                control.state = "uncheckedDisabled"
            }
        }
    }

    indicator: Rectangle {
        id: rectIndicator
        state: control.state
        implicitWidth: control.indicatorSize
        implicitHeight: control.indicatorSize
        x: control.leftPadding
        y: parent.height / 2 - height / 2
        radius: control.height / 2
        border.color: borderColorUncheckedHover
        border.width: 2

        states: [
            State {
                name: "unchecked"
                PropertyChanges {
                    target: rectIndicator
                    border.color: borderColorUnchecked
                    border.width: 2
                }
            },
            State {
                name: "uncheckedHover"
                PropertyChanges {
                    target: rectIndicator
                    border.color: borderColorUncheckedHover
                    border.width: 2
                }
            },
            State {
                name: "uncheckedDown"
                PropertyChanges {
                    target: rectIndicator
                    border.color: control.borderColorUncheckedHover
                    border.width: 2
                }
            },
            State {
                name: "uncheckedDisabled"
                PropertyChanges {
                    target: rectIndicator
                    border.color: borderColorUnchecked
                    border.width: 2
                }
            },
            State {
                name: "checked"
                PropertyChanges {
                    target: rectIndicator
                    border.color: borderColorChecked
                    border.width: 2
                }
            },
            State {
                name: "checkedHover"
                PropertyChanges {
                    target: rectIndicator
                    border.color: borderColorChecked
                    border.width: 2
                }
            },
            State {
                name: "checkedDown"
                PropertyChanges {
                    target: rectIndicator
                    border.color: borderColorChecked
                    border.width: 2
                }
            },
            State {
                name: "checkedDisabled"
                PropertyChanges {
                    target: rectIndicator
                    border.color: borderColorChecked
                    border.width: 2
                }
            }
        ]

        transitions: [
            Transition {
                from: "*"
                to: "*"

                NumberAnimation {target: rectIndicator; property: "border.width"; duration: 200}
                ColorAnimation {target: rectIndicator.border; duration: 200}
            }
        ]

        Rectangle {
            id: dot
            state: control.state
            width: 0
            height: 0
            x: control.indicatorSize / 2
            y: control.indicatorSize / 2
            radius: width / 2
            color: borderColorUnchecked

            states: [
                State {
                    name: "unchecked"
                    PropertyChanges {
                        target: dot
                        color: control.borderColorUnchecked
                        width: 0
                        height: 0
                        x: control.indicatorSize * 0.5
                        y: control.indicatorSize * 0.5
                    }
                },
                State {
                    name: "uncheckedHover"
                    PropertyChanges {
                        target: dot
                        color: control.borderColorUnchecked
                        width: control.indicatorSize * 0.45
                        height: control.indicatorSize * 0.45
                        x: control.indicatorSize * 0.275
                        y: control.indicatorSize * 0.275
                    }
                },
                State {
                    name: "uncheckedDown"
                    PropertyChanges {
                        target: dot
                        color: borderColorChecked
                        width: control.indicatorSize * 0.55
                        height: control.indicatorSize * 0.55
                        x: control.indicatorSize * 0.225
                        y: control.indicatorSize * 0.225
                    }
                },
                State {
                    name: "uncheckedDisabled"
                    PropertyChanges {
                        target: dot
                        color: control.borderColorUnchecked
                        width: 0
                        height: 0
                        x: control.indicatorSize * 0.5
                        y: control.indicatorSize * 0.5
                    }
                },
                State {
                    name: "checked"
                    PropertyChanges {
                        target: dot
                        color: borderColorChecked
                        width: control.indicatorSize * 0.45
                        height: control.indicatorSize * 0.45
                        x: control.indicatorSize * 0.275
                        y: control.indicatorSize * 0.275
                    }
                },
                State {
                    name: "checkedHover"
                    PropertyChanges {
                        target: dot
                        color: borderColorChecked
                        width: control.indicatorSize * 0.6
                        height: control.indicatorSize * 0.6
                        x: control.indicatorSize * 0.2
                        y: control.indicatorSize * 0.2
                    }
                },
                State {
                    name: "checkedDown"
                    PropertyChanges {
                        target: dot
                        color: borderColorUnchecked
                        width: control.indicatorSize * 0.4
                        height: control.indicatorSize * 0.4
                        x: control.indicatorSize * 0.3
                        y: control.indicatorSize * 0.3
                    }
                },
                State {
                    name: "checkedDisabled"
                    PropertyChanges {
                        target: dot
                        color: borderColorChecked
                        width: control.indicatorSize * 0.45
                        height: control.indicatorSize * 0.45
                        x: control.indicatorSize * 0.275
                        y: control.indicatorSize * 0.275
                    }
                }
            ]

            transitions: [
                Transition {
                    from: "*"
                    to: "*"
                    NumberAnimation {target: dot; duration: 200; property: "width"}
                    NumberAnimation {target: dot; duration: 200; property: "height"}
                    NumberAnimation {target: dot; duration: 200; property: "x"}
                    NumberAnimation {target: dot; duration: 200; property: "y"}
                    ColorAnimation {target: dot; duration: 200}
                }
            ]

        }

    }

    contentItem: Text {
        id: text
        text: control.text
        font: control.font
        color: "white"
        verticalAlignment: Text.AlignVCenter
        leftPadding: control.indicator.width + control.spacing

        NumberAnimation {
            target: text
            property: "opacity"
            duration: 200
        }
    }

    Component.onCompleted: updateState()
}
