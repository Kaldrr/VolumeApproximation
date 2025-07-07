import UserInterface

import QtCore
import QtQuick
import QtQuick3D
import QtQuick.Layouts
import QtQuick3D.Helpers
import QtQuick.Controls 2.15
import QtQuick.Dialogs
import QtQuick.Controls.Universal

ApplicationWindow {
    width: 1920
    height: 1080
    visible: true
    title: qsTr("Volume Approximation")
    Universal.theme: Universal.Light
    Universal.accent: Universal.Violet

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

            SceneEnvironment {
                id: sceneEnvironment
                antialiasingMode: SceneEnvironment.MSAA
                antialiasingQuality: SceneEnvironment.High
                backgroundMode: SceneEnvironment.Color
                clearColor: "#ffffff"
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
                    geometry: sceneController.mainModel
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
                    onClicked: fileDialog.open()
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
        mainModel: CuboidGeometry {
            id: cuboidGeometry
            xExtent: 100
            yExtent: 100
            zExtent: 100
        }
    }

    Item {
        id: __materialLibrary__
        PrincipledMaterial {
            id: defaultMaterial
            objectName: "Default Material"
            baseColor: "#4aee45"
        }
    }

    FileDialog {
        id: fileDialog
        currentFolder: StandardPaths.standardLocations(StandardPaths.PicturesLocation)[0]
        onAccepted: {
            sceneController.onLoadMeshButtonPress(selectedFile)
        }
    }
}
