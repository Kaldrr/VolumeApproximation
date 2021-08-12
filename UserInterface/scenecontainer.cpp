#include "ui_scenecontainer.h"

#include <UserInterface/scenecontainer.h>

SceneContainer::SceneContainer() : SceneContainer{nullptr} {};

SceneContainer::SceneContainer(QWidget *parent)
    : QWidget{parent}, m_ui{std::make_unique<Ui::SceneContainer>()},
      m_3dWindow{nullptr /*, Qt3DRender::API::Vulkan*/} {
  m_ui->setupUi(this);
  layout()->addWidget(QWidget::createWindowContainer(&m_3dWindow, this));

  //  m_3dWindow.show();
}

SceneContainer::~SceneContainer() = default;
