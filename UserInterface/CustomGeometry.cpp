#include <UserInterface/CustomGeometry.hpp>
#include <UserInterface/GeometryConstants.hpp>

#include <assimp/DefaultLogger.hpp>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <fmt/base.h>
#include <fmt/format.h>

#include <QVector3D>

#include <ranges>

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
    : CustomGeometry{ nullptr, modelPath }
{
}

CustomGeometry::CustomGeometry(QQuick3DObject* const parent,
                               const std::filesystem::path& modelPath)
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
	if (meshes.size() > 1)
	{
		fmt::println(
		    stdout,
		    "Model has more than one mesh, only the first one will be used.");
	}

	const aiMesh& mesh{ *meshes.front() };

	SetupVertexData(mesh);
	SetupIndexData(mesh);

	setBounds(std::bit_cast<QVector3D>(mesh.mAABB.mMin),
	          std::bit_cast<QVector3D>(mesh.mAABB.mMax));
}

void CustomGeometry::SetupVertexData(const aiMesh& mesh)
{
	// Use std::span to avoid pointer arithmetic
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
		// Positon  [x, y, z]
		// Normal   [x, y, z]
		// TexCoord [u, v]
		// 8 floats total per vertex
		const std::array<aiVector3D, 3> buffer{ vertices[idx], normals[idx],
			                                    texCoords[idx] };
		const auto offset{ static_cast<qsizetype>(idx * VertexSize *
			                                      sizeof(float)) };
		std::memcpy(&vertexData[offset], buffer.data(), sizeof(float) * VertexSize);
	}
	assert(vertexData.size() == totalSize);

	constexpr std::size_t PositionOffset{ 0 };
	constexpr std::size_t NormalOffset{ Constants::ElementsPerFace *
		                                sizeof(float) };
	constexpr std::size_t TexCoordOffset{ 2 * Constants::ElementsPerFace *
		                                  sizeof(float) };

	setPrimitiveType(PrimitiveType::Triangles);
	addAttribute(Attribute::PositionSemantic, PositionOffset, Attribute::F32Type);
	addAttribute(Attribute::NormalSemantic, NormalOffset, Attribute::F32Type);
	addAttribute(Attribute::TexCoordSemantic, TexCoordOffset, Attribute::F32Type);
	setVertexData(vertexData);
	setStride(VertexSize * sizeof(float));
}

void CustomGeometry::SetupIndexData(const aiMesh& mesh)
{
	const std::span<const aiFace> faces{ mesh.mFaces, mesh.mNumFaces };

	// aiProcess_Triangulate => each face has exactly 3 indices
	const auto requiredSize{ static_cast<qsizetype>(
		faces.size() * Constants::ElementsPerFace * sizeof(std::uint32_t)) };

	QByteArray indexData{};
	indexData.reserve(requiredSize);
	for (const aiFace& face : faces)
	{
		const std::span<const std::uint32_t> indices{ face.mIndices,
			                                          face.mNumIndices };
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
		indexData.append(reinterpret_cast<const char*>(indices.data()),
		                 static_cast<qsizetype>(Constants::ElementsPerFace *
		                                        sizeof(std::uint32_t)));
	}
	assert(indexData.size() == requiredSize);

	addAttribute(Attribute::IndexSemantic, 0, Attribute::U32Type);
	setIndexData(indexData);
}