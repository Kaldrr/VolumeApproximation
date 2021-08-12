#pragma once

#include <QWidget>

#include <Qt3DWindow>

#include <memory>

namespace Ui {
class SceneContainer;
}

class SceneContainer : public QWidget {
  Q_OBJECT

public:
  SceneContainer();
  explicit SceneContainer(QWidget *parent);
  ~SceneContainer();

private:
  std::unique_ptr<Ui::SceneContainer> m_ui;
  Qt3DExtras::Qt3DWindow m_3dWindow{};
};
