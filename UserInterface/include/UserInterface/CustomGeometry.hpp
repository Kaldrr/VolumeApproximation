#pragma once

#include <QQuick3DGeometry>

#include <filesystem>
#include <utility>

class aiMesh;

class CustomGeometry : public QQuick3DGeometry
{
	Q_OBJECT

	// Not exposed to the QML directly
	// Set in the SceneController as QQuick3DGeometry instead

public:
	explicit CustomGeometry(const std::filesystem::path& modelPath);
	CustomGeometry(QQuick3DObject* parent, const std::filesystem::path& modelPath);
	~CustomGeometry() override = default;

	CustomGeometry(const CustomGeometry&)                = delete;
	CustomGeometry(CustomGeometry&&) noexcept            = delete;
	CustomGeometry& operator=(const CustomGeometry&)     = delete;
	CustomGeometry& operator=(CustomGeometry&&) noexcept = delete;

private:
	void SetupVertexData(const aiMesh& mesh);
	void SetupIndexData(const aiMesh& mesh);
};
