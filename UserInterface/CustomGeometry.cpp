#include <UserInterface/CustomGeometry.hpp>
#include <UserInterface/GeometryConstants.hpp>

#include <assimp/DefaultLogger.hpp>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <fmt/format.h>

#include <QVector3D>

namespace
{
constexpr std::uint32_t ImportFlags =
    aiProcess_JoinIdenticalVertices | aiProcess_Triangulate |
    aiProcess_ValidateDataStructure | aiProcess_ImproveCacheLocality |
    aiProcess_RemoveRedundantMaterials | aiProcess_FindInvalidData |
    aiProcess_GenUVCoords | aiProcess_OptimizeMeshes | aiProcess_OptimizeGraph |
    aiProcess_GenBoundingBoxes;
} // namespace

CustomGeometry::CustomGeometry(const std::filesystem::path& modelPath)
    : CustomGeometry{ modelPath, nullptr }
{
}

CustomGeometry::CustomGeometry(const std::filesystem::path& modelPath,
                               QQuick3DObject* const parent)
    : QQuick3DGeometry{ parent }
{
	if (!std::filesystem::exists(modelPath))
	{
		throw std::runtime_error{ fmt::format("Model file does not exist: {}",
			                                  modelPath.string()) };
	}

	Assimp::Importer importer{};
	const aiScene* const scene{ importer.ReadFile(modelPath, ImportFlags) };
	if (scene == nullptr)
	{
		throw std::runtime_error{ fmt::format("Failed to load the model: {}",
			                                  importer.GetErrorString()) };
	}
	const std::span<const aiMesh* const> meshes{ scene->mMeshes,
		                                         scene->mNumMeshes };
	if (meshes.empty())
	{
		throw std::runtime_error{ "The model has no meshes." };
	}

	QByteArray allVertexData{};
	QByteArray allIndexData{};
	aiVector3D boundsMin{ meshes.front()->mAABB.mMin };
	aiVector3D boundsMax{ meshes.front()->mAABB.mMax };
	std::uint32_t vertexOffset{ 0 };

	for (const aiMesh* const meshPtr : meshes)
	{
		allVertexData.append(BuildVertexData(*meshPtr));
		allIndexData.append(BuildIndexData(*meshPtr, vertexOffset));
		vertexOffset += meshPtr->mNumVertices;

		boundsMin.x = std::min(boundsMin.x, meshPtr->mAABB.mMin.x);
		boundsMin.y = std::min(boundsMin.y, meshPtr->mAABB.mMin.y);
		boundsMin.z = std::min(boundsMin.z, meshPtr->mAABB.mMin.z);
		boundsMax.x = std::max(boundsMax.x, meshPtr->mAABB.mMax.x);
		boundsMax.y = std::max(boundsMax.y, meshPtr->mAABB.mMax.y);
		boundsMax.z = std::max(boundsMax.z, meshPtr->mAABB.mMax.z);
	}

	ConfigureAttributes();
	setVertexData(allVertexData);
	setIndexData(allIndexData);
	setBounds(std::bit_cast<QVector3D>(boundsMin),
	          std::bit_cast<QVector3D>(boundsMax));
}

void CustomGeometry::ConfigureAttributes()
{
	constexpr std::size_t TexCoordsSize{ 2 };
	constexpr std::size_t VertexSize{ Constants::ElementsPerFace * 2 +
		                              TexCoordsSize };
	constexpr std::size_t PositionOffset{ 0 };
	constexpr std::size_t NormalOffset{ Constants::ElementsPerFace *
		                                sizeof(float) };
	constexpr std::size_t TexCoordOffset{ 2 * Constants::ElementsPerFace *
		                                  sizeof(float) };

	setPrimitiveType(PrimitiveType::Triangles);
	addAttribute(Attribute::PositionSemantic, PositionOffset, Attribute::F32Type);
	addAttribute(Attribute::NormalSemantic, NormalOffset, Attribute::F32Type);
	addAttribute(Attribute::TexCoordSemantic, TexCoordOffset, Attribute::F32Type);
	addAttribute(Attribute::IndexSemantic, 0, Attribute::U32Type);
	setStride(VertexSize * sizeof(float));
}

QByteArray CustomGeometry::BuildVertexData(const aiMesh& mesh)
{
	const std::span<const aiVector3D> vertices{ mesh.mVertices, mesh.mNumVertices };
	const std::span<const aiVector3D> normals{ mesh.mNormals, mesh.mNumVertices };
	const std::span<const aiVector3D> texCoords{ mesh.mTextureCoords[0],
		                                         mesh.mNumVertices };

	constexpr std::size_t TexCoordsSize{ 2 };
	constexpr std::size_t VertexSize{ Constants::ElementsPerFace * 2 +
		                              TexCoordsSize };
	const auto totalSize{ static_cast<qsizetype>(vertices.size() * VertexSize *
		                                         sizeof(float)) };
	QByteArray vertexData{ totalSize, Qt::Initialization::Uninitialized };
	for (std::size_t idx{ 0 }; idx < vertices.size(); ++idx)
	{
		// Position [x, y, z]
		// Normal   [x, y, z]
		// TexCoord [u, v]
		// 8 floats per vertex
		const std::array<aiVector3D, 3> buffer{ vertices[idx], normals[idx],
			                                    texCoords[idx] };
		const auto offset{ static_cast<qsizetype>(idx * VertexSize *
			                                      sizeof(float)) };
		std::memcpy(&vertexData[offset], buffer.data(), sizeof(float) * VertexSize);
	}
	assert(vertexData.size() == totalSize);
	return vertexData;
}

QByteArray CustomGeometry::BuildIndexData(const aiMesh& mesh,
                                          const std::uint32_t vertexOffset)
{
	const std::span<const aiFace> faces{ mesh.mFaces, mesh.mNumFaces };

	// aiProcess_Triangulate => each face has exactly 3 indices
	const auto requiredSize{ static_cast<qsizetype>(
		faces.size() * Constants::ElementsPerFace * sizeof(std::uint32_t)) };

	QByteArray indexData{};
	indexData.reserve(requiredSize);
	for (const aiFace& face : faces)
	{
		for (std::size_t i{ 0 }; i < Constants::ElementsPerFace; ++i)
		{
			const std::uint32_t index{ face.mIndices[i] + vertexOffset };
			// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
			indexData.append(reinterpret_cast<const char*>(&index), sizeof(index));
		}
	}
	assert(indexData.size() == requiredSize);
	return indexData;
}
