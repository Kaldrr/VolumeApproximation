#pragma once

#include <QQuick3DGeometry>

#include <utility>

class aiMesh;

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

	template <typename Self>
	[[nodiscard]] auto&& GetVertexData(this Self&& self) noexcept
	{
		return std::forward_like<Self>(self.m_VertexData);
	}

	template <typename Self>
	[[nodiscard]] auto&& GetIndexData(this Self&& self) noexcept
	{
		return std::forward_like<Self>(self.m_IndexData);
	}

private:
	void SetupVertexData(const aiMesh& mesh);
	void SetupIndexData(const aiMesh& mesh);

private:
	std::vector<float> m_VertexData;
	std::vector<std::uint32_t> m_IndexData;
};
