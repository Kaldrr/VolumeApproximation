// The style must be imported before any other QtQuick.Controls imports.
import QtQuick.Controls.Universal

import UserInterface

import QtCore
import QtQuick
import QtQuick3D
import QtQuick.Layouts
import QtQuick3D.Helpers
import QtQuick.Controls 2.15
import QtQuick.Dialogs

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
            background: Rectangle {
                color: "#f0f0f0"
            }

            ColumnLayout {
                spacing: 20
                anchors.verticalCenter: parent.verticalCenter
                Layout.alignment: Qt.AlignVCenter

                Button {
                    Layout.fillWidth: true

                    text: qsTr("Load mesh")
                    onClicked: fileDialog.open()
                }

                Button {
                    Layout.fillWidth: true
                    text: qsTr("Load texture")
                    onClicked: textureFileDialog.open()
                }

                Label {
                    Layout.fillWidth: true

                    text: qsTr("Sample count")
                }

                TextField {
                    id: sampleCountField
                    Layout.fillWidth: true

                    text: "1000"
                    validator: IntValidator {
                        bottom: 0
                    }
                }

                Label {
                    Layout.fillWidth: true

                    text: qsTr("Compute engine")
                }

                ComboBox {
                    id: engineSelection
                    Layout.fillWidth: true
                    model: sceneController.availableComputeEngines
                    currentIndex: 0
                    onActivated: {
                        sceneController.onComputeEngineChanged(currentIndex)
                    }
                }

                Button {
                    // Layout.alignment: Qt.AlignHCenter
                    Layout.fillWidth: true

                    text: qsTr("Calculate")
                    onClicked: {
                        sceneController.onCalculateButtonPress(sampleCountField.text)
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
            baseColorMap: Texture {
                id: defaultTexture
            }
        }
    }

    FileDialog {
        id: fileDialog
        currentFolder: StandardPaths.standardLocations(StandardPaths.PicturesLocation)[0]
        onAccepted: {
            sceneController.onLoadMeshButtonPress(selectedFile)
        }
    }

    FileDialog {
        id: textureFileDialog
        currentFolder: StandardPaths.standardLocations(StandardPaths.PicturesLocation)[0]
        onAccepted: {
            defaultMaterial.baseColor = "#ffffff"
            defaultTexture.source = selectedFile
        }
    }
}
