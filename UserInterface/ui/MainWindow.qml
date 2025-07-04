import UserInterface

import QtQuick
import QtQuick3D
import QtQuick.Layouts
import QtQuick3D.Helpers
import QtQuick.Controls 2.15

Window {
    width: 640
    height: 480
    visible: true
    title: qsTr("Hello World")

    RowLayout {
        anchors.fill: parent

        View3D {
            id: view3D
            environment: sceneEnvironment
            Layout.alignment: Qt.AlignLeft
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredWidth: parent.width * 0.8
            camera: sceneCamera
            focus: true
            // onFocus: {
            //     print("View3D focused")
            // }

            SceneEnvironment {
                id: sceneEnvironment
                antialiasingMode: SceneEnvironment.MSAA
                antialiasingQuality: SceneEnvironment.High
            }

            OrbitCameraController {
                id: cameraController
                camera: sceneCamera
                origin: scene
                enabled: true
                // minimumDistance: 100
                // maximumDistance: 500
            }

            Node {
                id: scene
                DirectionalLight {
                    id: directionalLight
                }

                PerspectiveCamera {
                    id: sceneCamera
                    lookAtNode: cubeModel
                    z: 350
                }

                Model {
                    id: cubeModel
                    eulerRotation.y: 45
                    eulerRotation.x: 30
                    materials: defaultMaterial
                    source: "#Cube"
                }
            }
        }


        Pane {
            id: toolbar
            Layout.fillHeight: true
            Layout.alignment: Qt.AlignRight
            Layout.maximumWidth: 200

            ColumnLayout {
                anchors.fill: parent
                Button {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.fillWidth: true

                    text: qsTr("Load mesh")
                    onClicked: {
                        sceneController.onLoadMeshButtonPress()
                    }
                }
                Label {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.fillWidth: true

                    text: qsTr("Sample count")
                }
                TextField {
                    id: sampleCountField
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignHCenter

                    text: "1000"
                    validator: IntValidator {
                        bottom: 0
                    }
                }

                Label {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.fillWidth: true

                    text: qsTr("Compute engine")
                }

                ComboBox {
                    id: engineSelection
                    Layout.alignment: Qt.AlignHCenter
                    Layout.fillWidth: true

                    model: sceneController.availableComputeEngines
                    currentIndex: 0
                }

                Button {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.fillWidth: true

                    text: qsTr("Calculate")
                    onClicked: {
                        sceneController.onCalculateButtonPress(engineSelection.currentIndex)
                    }
                }
            }
        }
    }


    SceneController {
        id: sceneController
    }

    Item {
        id: __materialLibrary__
        PrincipledMaterial {
            id: defaultMaterial
            objectName: "Default Material"
            baseColor: "#4aee45"
        }
    }
}
