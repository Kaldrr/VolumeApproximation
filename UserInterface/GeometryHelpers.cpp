#include <UserInterface/GeometryHelpers.hpp>

#include <random>

#include <fmt/base.h>

namespace
{
// We always use triangles in our geometry
constexpr std::size_t ElementsInVertex{ 3 };

struct Attributes
{
	QQuick3DGeometry::Attribute Position;
	QQuick3DGeometry::Attribute Index;
};

[[nodiscard]] Attributes GetAttributes(const QQuick3DGeometry& geometry) noexcept
{
	std::optional<QQuick3DGeometry::Attribute> positionAttribute{};
	std::optional<QQuick3DGeometry::Attribute> indexAttribute{};

	const int attributeCount = geometry.attributeCount();
	for (int i = 0; i < attributeCount; ++i)
	{
		const QQuick3DGeometry::Attribute attribute = geometry.attribute(i);
		if (attribute.semantic ==
		    QQuick3DGeometry::Attribute::Semantic::PositionSemantic)
		{
			positionAttribute = attribute;
		}
		else if (attribute.semantic ==
		         QQuick3DGeometry::Attribute::Semantic::IndexSemantic)
		{
			indexAttribute = attribute;
		}

		if (positionAttribute.has_value() && indexAttribute.has_value())
		{
			return Attributes{ .Position = *positionAttribute,
				               .Index    = *indexAttribute };
		}
	}
	fmt::println(stderr, "Failed to extract attributes from geometry, ");
	fmt::println(stderr, "HasPosition={}, HasIndex={}",
	             positionAttribute.has_value(), indexAttribute.has_value());
	// Every model should have at least position and index attributes.
	// ...right?
	std::unreachable();
}

template <typename InDataT, typename OutDataT>
[[nodiscard]] std::vector<OutDataT> ExtractDataHelper(const QByteArray& data,
                                                      const std::size_t offset,
                                                      const std::size_t stride)
{
	const std::span<const std::byte> dataView{
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
		reinterpret_cast<const std::byte*>(data.data()),
		static_cast<std::size_t>(data.size())
	};
	const std::size_t elementCount{ dataView.size() / stride };

	// We are always using triangles
	// So position will be stored as: [X, Y, Z, NX, NY, NZ, U, V]
	// Should always be float's
	// And indices as: [X, Y, Z]
	// But they can change, u16, u32, etc.
	std::vector<OutDataT> extractedData{};
	extractedData.resize(elementCount * 3);
	using ItrDiff = typename std::vector<OutDataT>::difference_type;
	for (std::size_t i = 0; i < elementCount; ++i)
	{
		// We MIGHT need to do a data conversion
		// Indices stored as u32, but we want u64 out etc.
		// memcpy would do the wrong thing in such case
		std::copy_n(
		    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
		    reinterpret_cast<const InDataT*>(&dataView[(i * stride) + offset]),
		    ElementsInVertex,
		    std::next(extractedData.begin(),
		              static_cast<ItrDiff>(i * ElementsInVertex)));
	}

	return extractedData;
}

template <typename OutDataT>
[[nodiscard]] std::vector<OutDataT> ExtractData(
    const QByteArray& data,
    const QQuick3DGeometry::Attribute& attribute,
    const std::size_t stride)
{
	const std::size_t offset{ static_cast<std::size_t>(attribute.offset) };
	switch (attribute.componentType)
	{
		case QQuick3DGeometry::Attribute::ComponentType::U16Type:
			return ExtractDataHelper<std::uint16_t, OutDataT>(data, offset, stride);
		case QQuick3DGeometry::Attribute::ComponentType::U32Type:
			return ExtractDataHelper<std::uint32_t, OutDataT>(data, offset, stride);
		case QQuick3DGeometry::Attribute::ComponentType::I32Type:
			return ExtractDataHelper<std::int32_t, OutDataT>(data, offset, stride);
		case QQuick3DGeometry::Attribute::ComponentType::F32Type:
			return ExtractDataHelper<float, OutDataT>(data, offset, stride);
	}
	fmt::println(stderr, "Unknown component type: {}",
	             static_cast<int>(attribute.componentType));
	std::unreachable();
}
} // namespace

namespace Helpers
{
[[nodiscard]]
std::vector<VolumeApproximation::Vector3F> GenerateRandomPoints(
    const std::size_t count,
    const VolumeApproximation::Vector3F& minExtent,
    const VolumeApproximation::Vector3F& maxExtent)
{
	std::vector<VolumeApproximation::Vector3F> randomPoints{};
	randomPoints.reserve(count);

	std::mt19937_64 dev{ std::random_device{}() };
	std::uniform_real_distribution<float> rngX{ minExtent[0], maxExtent[0] };
	std::uniform_real_distribution<float> rngY{ minExtent[1], maxExtent[1] };
	std::uniform_real_distribution<float> rngZ{ minExtent[2], maxExtent[2] };

	for (std::size_t i = 0; i < count; ++i)
	{
		randomPoints.emplace_back(rngX(dev), rngY(dev), rngZ(dev));
	}
	return randomPoints;
}

[[nodiscard]]
std::vector<VolumeApproximation::Triangle> ParseTrianglesFromGeometry(
    const QQuick3DGeometry& geometry)
{
	const auto stride{ static_cast<std::size_t>(geometry.stride()) };
	const Attributes attributes{ GetAttributes(geometry) };

	const std::vector<float> vertexData{ ExtractData<float>(
		geometry.vertexData(), attributes.Position, stride) };
	const std::vector<std::uint32_t> indexData{ ExtractData<std::uint32_t>(
		geometry.indexData(), attributes.Index, stride) };

	std::vector<VolumeApproximation::Triangle> triangles{};
	triangles.reserve(vertexData.size() / ElementsInVertex);

	for (std::size_t i{ 0 }; i < indexData.size(); i += 3)
	{
		const std::uint64_t xIdx{ indexData[i] };
		const std::uint64_t yIdx{ indexData[i + 1] };
		const std::uint64_t zIdx{ indexData[i + 2] };
		const VolumeApproximation::Triangle triangle{
			VolumeApproximation::Vector3F{ vertexData[xIdx], vertexData[xIdx + 1],
			                               vertexData[xIdx + 2] },
			VolumeApproximation::Vector3F{ vertexData[yIdx], vertexData[yIdx + 1],
			                               vertexData[yIdx + 2] },
			VolumeApproximation::Vector3F{ vertexData[zIdx], vertexData[zIdx + 1],
			                               vertexData[zIdx + 2] }
		};
		triangles.emplace_back(triangle);
	}
	assert(triangles.size() == indexData.size() / 3);

	return triangles;
}
} // namespace Helpers