#include <UserInterface/CustomGeometry.hpp>
#include <UserInterface/GeometryConstants.hpp>
#include <UserInterface/GeometryHelpers.hpp>

#include <cassert>
#include <concepts>
#include <cstdint>
#include <cstring>
#include <optional>
#include <random>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#include <fmt/format.h>

namespace
{
struct Attributes
{
	QQuick3DGeometry::Attribute Position;
	QQuick3DGeometry::Attribute Index;
};

[[nodiscard]] Attributes GetAttributes(const QQuick3DGeometry& geometry)
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
	throw std::runtime_error{ fmt::format(
		"Failed to extract attributes from geometry, HasPosition={}, HasIndex={}",
		positionAttribute.has_value(), indexAttribute.has_value()) };
}

// Position attribute is [X, Y, Z]
constexpr std::size_t PositionComponents{ 3 };

using ComponentType = QQuick3DGeometry::Attribute::ComponentType;

/// Calls `visitor.template operator()<T>()`, where T is the C++ type stored
/// for `type` (e.g. U32Type -> std::uint32_t). Turns the runtime component
/// type into a compile-time one, in a single place.
template <typename Visitor>
[[nodiscard]] decltype(auto) VisitComponentType(const ComponentType type,
                                                Visitor&& visitor)
{
	switch (type)
	{
		case ComponentType::U16Type:
			return std::forward<Visitor>(visitor)
			    .template operator()<std::uint16_t>();
		case ComponentType::U32Type:
			return std::forward<Visitor>(visitor)
			    .template operator()<std::uint32_t>();
		case ComponentType::I32Type:
			return std::forward<Visitor>(visitor)
			    .template operator()<std::int32_t>();
		case ComponentType::F32Type:
			return std::forward<Visitor>(visitor).template operator()<float>();
	}
	throw std::runtime_error{ fmt::format("Unknown geometry component type: {}",
	                                      std::to_underlying(type)) };
}

[[nodiscard]] std::span<const std::byte> AsBytes(const QByteArray& data) noexcept
{
	return std::as_bytes(
	    std::span{ data.constData(), static_cast<std::size_t>(data.size()) });
}

/// Converts one component, rejecting negative values that would wrap when
/// stored in an unsigned type.
template <typename InT, typename OutT>
[[nodiscard]] OutT ConvertComponent(const InT value)
{
	if constexpr (std::is_signed_v<InT> && std::is_unsigned_v<OutT>)
	{
		if (value < 0)
		{
			throw std::runtime_error{ fmt::format(
				"Negative value {} can't be stored as unsigned", value) };
		}
	}
	return static_cast<OutT>(value);
}

/// Reads `componentCount` consecutive InT values from every element of an
/// interleaved buffer (element i starts at i * stride + offset) and converts
/// them to OutT, returning a flat array of components.
template <typename InT, typename OutT>
    requires std::is_arithmetic_v<InT> && std::is_arithmetic_v<OutT>
[[nodiscard]] std::vector<OutT> ReadComponents(
    const std::span<const std::byte> bytes,
    const std::size_t offset,
    const std::size_t stride,
    const std::size_t componentCount)
{
	const std::size_t elementSize{ componentCount * sizeof(InT) };
	if (stride == 0 || offset + elementSize > stride)
	{
		throw std::runtime_error{ fmt::format(
			"Attribute (offset {}, {} bytes) doesn't fit in stride {}", offset,
			elementSize, stride) };
	}
	// The last element may not be padded to a full stride
	const std::size_t elementCount{
		bytes.size() < offset + elementSize
		    ? 0
			: ((bytes.size() - offset - elementSize) / stride) + 1
	};

	std::vector<OutT> components{};
	if constexpr (std::is_same_v<InT, OutT>)
	{
		// Tightly packed and no conversion needed: copy the whole block at once
		if (stride == elementSize)
		{
			components.resize(elementCount * componentCount);
			std::memcpy(components.data(), bytes.subspan(offset).data(),
			            components.size() * sizeof(OutT));
			return components;
		}
	}

	components.reserve(elementCount * componentCount);
	for (std::size_t element{ 0 }; element < elementCount; ++element)
	{
		const std::span<const std::byte> elementBytes{ bytes.subspan(
			(element * stride) + offset, elementSize) };
		for (std::size_t component{ 0 }; component < componentCount; ++component)
		{
			// memcpy: buffer data isn't guaranteed to be aligned for InT
			InT value{};
			std::memcpy(&value,
			            elementBytes.subspan(component * sizeof(InT)).data(),
			            sizeof(InT));
			components.push_back(ConvertComponent<InT, OutT>(value));
		}
	}
	return components;
}

/// Flat components of an interleaved vertex attribute (positions: X0, Y0, Z0,
/// X1...)
template <typename OutT>
[[nodiscard]] std::vector<OutT> ExtractVertexAttribute(
    const QByteArray& vertexData,
    const QQuick3DGeometry::Attribute& attribute,
    const std::size_t stride,
    const std::size_t componentCount)
{
	return VisitComponentType(attribute.componentType, [&]<typename InT>() {
		return ReadComponents<InT, OutT>(AsBytes(vertexData),
		                                 static_cast<std::size_t>(attribute.offset),
		                                 stride, componentCount);
	});
}

/// Index buffer is tightly packed, one index per element
template <std::integral OutT>
[[nodiscard]] std::vector<OutT> ExtractIndices(
    const QByteArray& indexData,
    const QQuick3DGeometry::Attribute& attribute)
{
	return VisitComponentType(attribute.componentType, [&]<typename InT>() {
		if constexpr (std::floating_point<InT>)
		{
			throw std::runtime_error{ "Index buffer must use an integer type" };
			return std::vector<OutT>{};
		}
		else
		{
			const std::span<const std::byte> bytes{ AsBytes(indexData) };
			const auto offset{ static_cast<std::size_t>(attribute.offset) };
			if (offset > bytes.size())
			{
				throw std::runtime_error{ fmt::format(
					"Index offset {} past end of {}-byte buffer", offset,
					bytes.size()) };
			}
			return ReadComponents<InT, OutT>(bytes.subspan(offset), 0, sizeof(InT),
			                                 1);
		}
	});
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

	const std::vector<float> vertexData{ ExtractVertexAttribute<float>(
		geometry.vertexData(), attributes.Position, stride, PositionComponents) };

	const std::vector<std::uint32_t> indexData{ ExtractIndices<std::uint32_t>(
		geometry.indexData(), attributes.Index) };

	if (indexData.size() % Constants::ElementsPerFace != 0)
	{
		throw std::runtime_error{ fmt::format(
			"Index count {} isn't a multiple of {} (triangles only)",
			indexData.size(), Constants::ElementsPerFace) };
	}
	const std::size_t vertexCount{ vertexData.size() / PositionComponents };
	const auto vertexAt{ [&](const std::uint32_t index) {
		if (index >= vertexCount)
		{
			throw std::runtime_error{ fmt::format(
				"Vertex index {} out of range ({} vertices)", index, vertexCount) };
		}
		const std::size_t base{ index * PositionComponents };
		return VolumeApproximation::Vector3F{ vertexData[base],
		                                      vertexData[base + 1ZU],
		                                      vertexData[base + 2ZU] };
	} };

	std::vector<VolumeApproximation::Triangle> triangles{};
	triangles.reserve(indexData.size() / Constants::ElementsPerFace);
	for (std::size_t i{ 0 }; i < indexData.size(); i += Constants::ElementsPerFace)
	{
		triangles.push_back(VolumeApproximation::Triangle{
		    vertexAt(indexData[i]), vertexAt(indexData[i + 1]),
		    vertexAt(indexData[i + 2]) });
	}
	assert(triangles.size() == indexData.size() / Constants::ElementsPerFace);

	return triangles;
}
} // namespace Helpers