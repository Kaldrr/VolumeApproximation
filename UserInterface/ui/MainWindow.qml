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

            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.RightButton
                property real lastX: 0
                property real lastY: 0
                property bool rotating: false

                onPressed: (mouseEvent) => {
                    if (mouseEvent.button === Qt.RightButton) {
                        lastX = mouseEvent.x;
                        lastY = mouseEvent.y;
                        rotating = true;
                    }
                }

                onReleased: (mouseEvent) => {
                    if (mouseEvent.button === Qt.RightButton) {
                        rotating = false;
                    }
                }

                onPositionChanged: (mouseEvent) => {
                    if (rotating) {
                        const dx = mouseEvent.x - lastX;
                        const dy = mouseEvent.y - lastY;
                        rootNode.eulerRotation.y += dx * 0.5;
                        rootNode.eulerRotation.x += dy * 0.5;
                        lastX = mouseEvent.x;
                        lastY = mouseEvent.y;
                    }
                }
            }

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

                PerspectiveCamera {
                    id: sceneCamera
                    lookAtNode: modelNode
                    z: 350
                }

                Model {
                    id: rootNode
                    Model {
                        id: modelNode
                        materials: modelMaterial
                        geometry: sceneController.mainModel
                    }

                    Model {
                        id: pointsModel
                        geometry: SphereGeometry {
                            radius: 0.01
                        }
                        materials: defaultMaterial
                        Component.onCompleted: {
                            // Silence QML warnings about non-bindable properties
                            // ... but is this any better really?
                            pointsModel.instancing = sceneController.pointInstanceModel;
                        }
                    }
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
                    model: ListModel {
                    }
                    currentIndex: 0
                    onActivated: {
                        sceneController.onComputeEngineChanged(currentIndex)
                    }
                    Component.onCompleted: {
                        // Explicit model to silence QML warnings
                        // About non-bindable properties >:(
                        for (let engine of sceneController.availableComputeEngines) {
                            engineSelection.model.append({ "name": engine });
                        }
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

                Button {
                    Layout.fillWidth: true
                    text: qsTr("Toggle Model")
                    onClicked: modelNode.visible = !modelNode.visible
                }

                // --- Volume Result ---
                Rectangle {
                    Layout.fillWidth: true
                    color: "transparent"
                    border.color: "#cccccc"
                    radius: 4
                    height: volumeResultColumn.implicitHeight + 16

                    ColumnLayout {
                        id: volumeResultColumn
                        anchors {
                            left: parent.left
                            right: parent.right
                            top: parent.top
                            margins: 8
                        }
                        spacing: 4

                        Label {
                            text: qsTr("Volume Result")
                            font.bold: true
                        }

                        Label {
                            Layout.fillWidth: true
                            text: sceneController.computedVolume > 0
                                  ? sceneController.computedVolume.toFixed(2) + " cm³"
                                  : qsTr("—")
                            font.pixelSize: 14
                            wrapMode: Text.WordWrap
                        }
                    }
                }
                // --- End Volume Result ---

                // --- Points Instancing Controls ---
                Rectangle {
                    Layout.fillWidth: true
                    color: "transparent"
                    border.color: "#cccccc"
                    radius: 4
                    ColumnLayout {
                        spacing: 8
                        anchors.fill: parent

                        Label {
                            text: qsTr("Points Instancing")
                            font.bold: true
                        }

                        CheckBox {
                            id: pointsVisibleBox
                            text: qsTr("Show Points")
                            checked: pointsModel.visible
                            onCheckedChanged: pointsModel.visible = checked
                        }

                        RowLayout {
                            spacing: 4
                            Label {
                                text: qsTr("Sphere Size")
                            }
                            Slider {
                                id: sphereSizeSlider
                                from: 0.005
                                to: 0.5
                                stepSize: 0.001
                                value: pointsModel.geometry.radius
                                Layout.fillWidth: true
                                onValueChanged: pointsModel.geometry.radius = value
                            }
                        }
                    }
                }
                // --- End Points Instancing Controls ---
            }
        }
    }

    SceneController {
        id: sceneController
        onRepositionCamera: (pos) => {
            sceneCamera.position = pos;
        }
        onMainModelChanged: {
            modelNode.eulerRotation = Qt.vector3d(0, 0, 0);
        }
    }

    Item {
        id: __materialLibrary__

        PrincipledMaterial {
            id: modelMaterial
            objectName: "Default Material"
            baseColor: "#4aee45"
            baseColorMap: Texture {
                id: modelTexture
            }
            lighting: DefaultMaterial.NoLighting
        }

        PrincipledMaterial {
            id: defaultMaterial
            objectName: "Default Material"
            baseColor: "#4aee45"
            baseColorMap: Texture {
                id: defaultTexture
            }
            lighting: DefaultMaterial.NoLighting
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
            modelMaterial.baseColor = "#ffffff"
            modelTexture.source = selectedFile
        }
    }
}
