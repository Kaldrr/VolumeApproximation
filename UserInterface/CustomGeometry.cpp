#include <CustomGeometry.hpp>

#include <assimp/DefaultLogger.hpp>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <fmt/base.h>

#include <QVector3D>

namespace
{
constexpr std::uint32_t ImportFlags =
    aiProcess_JoinIdenticalVertices | aiProcess_Triangulate |
    aiProcess_ValidateDataStructure | aiProcess_ImproveCacheLocality |
    aiProcess_RemoveRedundantMaterials | aiProcess_FindInvalidData |
    aiProcess_GenUVCoords | aiProcess_OptimizeMeshes | aiProcess_OptimizeGraph |
    aiProcess_GenBoundingBoxes
    // | aiProcess_FlipUVs
    ;

} // namespace

CustomGeometry::CustomGeometry(const std::string& modelPath)
    : CustomGeometry{ nullptr, modelPath }
{
}

CustomGeometry::CustomGeometry(QQuick3DObject* const parent,
                               const std::string& modelPath)
    : QQuick3DGeometry{ parent }
{
	Assimp::Importer importer{};
	const aiScene* const scene{ importer.ReadFile(modelPath, ImportFlags) };
	if (scene == nullptr)
	{
		throw std::runtime_error{ "Failed to load the model: " +
			                      std::string{ importer.GetErrorString() } };
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

	const aiVector3D& minBox = mesh.mAABB.mMin;
	const aiVector3D& maxBox = mesh.mAABB.mMax;
	setBounds(QVector3D{ minBox.x, minBox.y, minBox.z },
	          QVector3D{ maxBox.x, maxBox.y, maxBox.z });
}

void CustomGeometry::SetupVertexData(const aiMesh& mesh)
{
	const std::span<const aiVector3D> vertices{ mesh.mVertices, mesh.mNumVertices };
	const std::span<const aiVector3D> normals{ mesh.mNormals, mesh.mNumVertices };
	const std::span<const aiVector3D> texCoords{ mesh.mTextureCoords[0],
		                                         mesh.mNumVertices };

	constexpr std::size_t VertexSize{ 3 + 3 + 2 };
	const std::size_t vertexDataSize{ vertices.size() * sizeof(aiVector3D) *
		                              VertexSize };
	QByteArray vertexData{ static_cast<qsizetype>(vertexDataSize),
		                   Qt::Initialization::Uninitialized };
	const std::span<float> vertexDataSpan{
		reinterpret_cast<float*>(vertexData.data()), vertexDataSize

	};
	std::size_t vertexIdx{ 0 };
	for (std::size_t idx{ 0 }; idx < vertices.size(); ++idx)
	{
		// position - 3 floats
		// normal - 3 floats
		// texCoord - 2 floats
		const std::array<aiVector3D, 3> buffer{
			vertices[idx], normals[idx], texCoords[idx]
		};
		std::memcpy(&vertexDataSpan[vertexIdx], buffer.data(), sizeof(float) * VertexSize);
		vertexIdx += VertexSize;
	}

	setPrimitiveType(QQuick3DGeometry::PrimitiveType::Triangles);

	addAttribute(QQuick3DGeometry::Attribute::PositionSemantic, 0,
	             QQuick3DGeometry::Attribute::F32Type);
	addAttribute(QQuick3DGeometry::Attribute::NormalSemantic, 3 * sizeof(float),
	             QQuick3DGeometry::Attribute::F32Type);
	addAttribute(QQuick3DGeometry::Attribute::TexCoordSemantic, 6 * sizeof(float),
	             QQuick3DGeometry::Attribute::F32Type);
	setVertexData(vertexData);
	setStride(VertexSize * sizeof(float));
}

void CustomGeometry::SetupIndexData(const aiMesh& mesh)
{
	const std::span<const aiFace> faces{ mesh.mFaces, mesh.mNumFaces };
	QByteArray indexData{ static_cast<qsizetype>(faces.size() * sizeof(aiFace)),
		                  Qt::Initialization::Uninitialized };
	for (auto* indexDataPtr = reinterpret_cast<std::uint32_t*>(indexData.data());
	     const aiFace& face : faces)
	{
		std::memcpy(indexDataPtr, face.mIndices, sizeof(std::uint32_t) * 3);
		indexDataPtr += 3;
	}
	addAttribute(QQuick3DGeometry::Attribute::IndexSemantic, 0,
	             QQuick3DGeometry::Attribute::U32Type);
	setIndexData(indexData);
}