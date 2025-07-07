#pragma once

#include <QQuick3DGeometry>

class CustomGeometry : public QQuick3DGeometry
{
	Q_OBJECT

	// Not exposed to the QML directly
	// Set in the SceneController instead

public:
	explicit CustomGeometry(const std::string& modelPath);
	CustomGeometry(QQuick3DObject* parent, const std::string& modelPath);
	~CustomGeometry() override = default;

	CustomGeometry(const CustomGeometry&)                = delete;
	CustomGeometry(CustomGeometry&&) noexcept            = delete;
	CustomGeometry& operator=(const CustomGeometry&)     = delete;
	CustomGeometry& operator=(CustomGeometry&&) noexcept = delete;


private:
};
