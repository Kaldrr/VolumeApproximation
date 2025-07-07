#include <CustomGeometry.hpp>

#include <assimp/DefaultLogger.hpp>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <fmt/base.h>

namespace
{
constexpr std::uint32_t ImportFlags =
    aiProcess_JoinIdenticalVertices | aiProcess_Triangulate |
    aiProcess_ValidateDataStructure | aiProcess_ImproveCacheLocality |
    aiProcess_RemoveRedundantMaterials | aiProcess_FindInvalidData |
    aiProcess_GenUVCoords | aiProcess_OptimizeMeshes | aiProcess_OptimizeGraph
    // | aiProcess_FlipUVs
    ;

template <typename... Functors>
// NOLINTNEXTLINE(fuchsia-multiple-inheritance)
struct [[nodiscard]] Overload : Functors...
{
	using Functors::operator()...;
};
} // namespace

CustomGeometry::CustomGeometry(const std::string& modelPath)
    : CustomGeometry{ nullptr, modelPath }
{
}

CustomGeometry::CustomGeometry(QQuick3DObject* const parent, const std::string& modelPath)
    : QQuick3DGeometry{ parent }
{
	Assimp::Importer importer{};
	const aiScene* const scene = importer.ReadFile(modelPath, ImportFlags);
	if (scene == nullptr)
	{
		throw std::runtime_error{ "Failed to load the model: " +
			                      std::string{ importer.GetErrorString() } };
	}
	const std::span<const aiMesh* const> meshes{
		scene->mMeshes, static_cast<std::size_t>(scene->mNumMeshes)
	};
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

	const aiMesh* const mesh = meshes.front();
	const std::span<const aiVector3D> vertices{
		mesh->mVertices, static_cast<std::size_t>(mesh->mNumVertices)
	};
	const std::span normals{
		mesh->mNormals, static_cast<std::size_t>(mesh->mNumVertices)
	};
	const std::span texCoords{
		mesh->mTextureCoords[0], static_cast<std::size_t>(mesh->mNumVertices)
	};

	constexpr std::size_t VertexDataSize = 3 + 3 + 2;
	QByteArray vertexData{ static_cast<qsizetype>(vertices.size() * sizeof(aiVector3D) * VertexDataSize),
		                   Qt::Initialization::Uninitialized };
	const std::span<float> vertexDataSpan{
		reinterpret_cast<float*>(vertexData.data()),
		vertices.size()};
	std::size_t vertexIdx{0};
	for (std::size_t idx{0}; idx < vertices.size(); ++idx)
	{
		// position - 3 floats
		// normal - 3 floats
		// texCoord - 2 floats
		std::memcpy(
		    &vertexDataSpan[vertexIdx], &vertices[idx],
		    sizeof(aiVector3D));
		std::memcpy(
		    &vertexDataSpan[vertexIdx + 3], &normals[idx],
		    sizeof(aiVector3D));
		std::memcpy(
		    &vertexDataSpan[vertexIdx + 6], &texCoords[idx],
		    sizeof(aiVector3D::x) * 2); // Only x and y are used for texture coords

		vertexIdx += VertexDataSize;
	}

	const std::span<const aiFace> faces{ mesh->mFaces, static_cast<std::size_t>(
														   mesh->mNumFaces) };
	QByteArray indexData{
	    static_cast<qsizetype>(faces.size() * sizeof(aiFace)), Qt::Initialization::Uninitialized};
	auto* indexDataPtr = reinterpret_cast<std::uint32_t*>(indexData.data());
	for (const aiFace& face : faces)
	{
		std::memcpy(indexDataPtr, face.mIndices, sizeof(std::uint32_t) * 3);
		indexDataPtr += 3;
	}

	setPrimitiveType(QQuick3DGeometry::PrimitiveType::Triangles);

	addAttribute(QQuick3DGeometry::Attribute::PositionSemantic, 0,
	             QQuick3DGeometry::Attribute::F32Type);
	addAttribute(QQuick3DGeometry::Attribute::NormalSemantic, 3 * sizeof(float),
	             QQuick3DGeometry::Attribute::F32Type);
	addAttribute(QQuick3DGeometry::Attribute::TexCoordSemantic, 6 * sizeof(float),
	             QQuick3DGeometry::Attribute::F32Type);
	setVertexData(vertexData);
	setStride(VertexDataSize * sizeof(float));

	addAttribute(QQuick3DGeometry::Attribute::IndexSemantic, 0,
	             QQuick3DGeometry::Attribute::U32Type);
	setIndexData(indexData);

}