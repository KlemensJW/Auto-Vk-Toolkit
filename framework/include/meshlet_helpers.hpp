﻿#pragma once
#include <gvk.hpp>

namespace gvk
{
	/** Meshlet struct for the CPU side. */
	struct meshlet
	{
		/** The model of the meshlet. */
		model mModel;
		/** The optional mesh index of the meshlet.
		 *  Only set if the submeshes were not combined upon creation of the meshlet.
		 */
		std::optional<mesh_index_t> mMeshIndex;
		/** Contains indices into the original vertex attributes of the mesh.
		 *  If submeshes were combined, it indexes the vertex attributes of the combined meshes as done with get_vertices_and_indices.
		 */
		std::vector<uint32_t> mVertices;
		/** Contains indices into the mVertices vector. */
		std::vector<uint8_t> mIndices;
		/** The actual number of vertices inside of mVertices; */
		uint32_t mVertexCount;
		/** The actual number of indices in mIndices; */
		uint32_t mIndexCount;
	};

	/** Meshlet for GPU usage
	 *  @tparam NV	The number of vertices
	 *	@tparam NI	The number of indices
	 */
	template <size_t NV = 64, size_t NI = 378>
	struct meshlet_gpu_data
	{
		static const size_t sNumVertices = NV;
		static const size_t sNumIndices = NI;

		/** Vertex indices into the vertex array */
		uint32_t mVertices[NV];
		/** Indices into the vertex indices */
		uint8_t mIndices[NI];  
		/** The vertex count */
		uint8_t mVertexCount;
		/** The primitive count */
		uint8_t mPrimitiveCount;
	};

	/** Meshlet for GPU usage in combination with the meshlet data generated by convert_for_gpu_usage */
	struct meshlet_redirected_gpu_data
	{
		/** Data offset into the meshlet data array */
		uint32_t mDataOffset;
		/** The vertex count */
		uint8_t mVertexCount;
		/** The primitive count */
		uint8_t mPrimitiveCount;
	};

	/** Serialization/deserialization method for meshlet_gpu_data.
	 *	@param	aArchive	The archive.
	 *	@param	aValue		The value to serialize or to deserialize into.
	 *	@tparam Archive		The archive type.
	 *	@tparam NV			The number of vertices used by the meshlet_gpu_data.
	 *	@tparam NI			The number of indices used by the meshlet_gpu_data.
	 */
	template<typename Archive, size_t NV, size_t NI>
	void serialize(Archive& aArchive, meshlet_gpu_data<NV, NI>& aValue)
	{
		aArchive(gvk::serializer::binary_data(aValue.mVertices, sizeof(meshlet_gpu_data<NV, NI>::mVertices)));
		aArchive(gvk::serializer::binary_data(aValue.mVertices, sizeof(meshlet_gpu_data<NV, NI>::mIndices)));
		aArchive(aValue.mVertexCount, aValue.mPrimitiveCount);
	}

	/** Serialization/deserialization method for meshlet_redirected_gpu_data.
	 *	@param	aArchive	The archive.
	 *	@param	aValue		The value to serialize or to deserialize into.
	 *	@tparam Archive		The archive type.
	 */
	template<typename Archive>
	void serialize(Archive& aArchive, meshlet_redirected_gpu_data& aValue)
	{
		aArchive(aValue.mDataOffset, aValue.mVertexCount, aValue.mPrimitiveCount);
	}


	/** Divides the given index buffer into meshlets by simply aggregating every aMaxVertices indices into a meshlet.
	 *  @param	aIndices			The index buffer.
	 *  @param	aModel				The model these buffers belong to.
	 *	@param	aMeshIndex			The optional mesh index of the mesh these buffers belong to.
	 *	@param	aMaxVertices		The maximum number of vertices of a meshlet.
	 *	@param	aMaxIndices			The maximum number of indices of a meshlet.
	 */
	std::vector<meshlet> basic_meshlets_divider(
		const std::vector<uint32_t>& aIndices,
		const model_t& aModel,
		std::optional<mesh_index_t> aMeshIndex,
		uint32_t aMaxVertices, uint32_t aMaxIndices);

	/** Divides the given vertex and index buffer into meshlets using the given callback function.
	 *  @param	aVertices			The vertex buffer.
	 *  @param	aIndices			The index buffer.
	 *  @param	aModel				The model these buffers belong to.
	 *	@param	aMeshIndex			The optional mesh index of the mesh these buffers belong to.
	 *  @param	aMeshletDivision	Callback used to divide meshes into meshlets with a maximum number of vertices and indices each.
	 *								It can either receive the vertices and indices or just the indices depending on your specific needs.
	 *								Additionally it provides the model and an optional mesh index if more data is needed. If no mesh index is provided then the meshes were combined beforehand.
	 *								Ownership of the model must not be taken within the body of aMeshletDivision. The model will be assigned to each `meshlet` after aMeshletDivision has executed. \n\n
	 *								The callback must follow a specific declaration schema, optional parameters can be omitted, but all of them need to be provided in the following order:
	 *								 - const std::vector<glm::vec3>& tVertices:		optional	The vertices of the mesh or combined meshes of the model
	 *								 - const std::vector<uint32_t>& tIndices:  		mandatory	The indices of the mesh or combined meshes of the model
	 *								 - const model_t& tModel:						mandatory	The model these meshlets are generated from
	 *								 - std::optional<mesh_index_t> tMeshIndex: 		mandatory	The optional mesh index. If no value is provided, it means the meshes of the model are combined into a single vertex and index buffer
	 *								 - uint32_t tMaxVertices:						mandatory	The maximum number of vertices that are allowed in a single meshlet.
	 *								 - uint32_t tMaxIndices:						mandatory	The maximum number of indices that are allowed in a single meshlet.
	 *								Return value:
	 *								 - std::vector<meshlet>										The generated meshlets.\n\n
	 *								Example:
	 *								@code
	 *								[](const std::vector<glm::vec3>& tVertices, const std::vector<uint32_t>& aIndices,
	 *											const model_t& aModel, std::optional<mesh_index_t> aMeshIndex,
	 *											uint32_t aMaxVertices, uint32_t aMaxIndices) {
	 *									std::vector<gvk::meshlet> generatedMeshlets;
	 *									// Do your meshlet division here
	 *									return generatedMeshlets;
	 *								}
	 *								@endcode
	 *	@param	aMaxVertices		The maximum number of vertices of a meshlet. This value is just passed on to aMeshletDivision.
	 *	@param	aMaxIndices			The maximum number of indices of a meshlet. This value is just passed on to aMeshletDivision.
	 */
	template <typename F>
	std::vector<meshlet> divide_indexed_geometry_into_meshlets(
		const std::vector<glm::vec3>& aVertices,
		const std::vector<uint32_t>& aIndices,
		gvk::model aModel,
		const std::optional<mesh_index_t> aMeshIndex,
		const uint32_t aMaxVertices, const uint32_t aMaxIndices,
		F aMeshletDivision)
	{
		std::vector<meshlet> generatedMeshlets;
		aModel.enable_shared_ownership();

		if constexpr (std::is_assignable_v<std::function<std::vector<meshlet>(const std::vector<uint32_t>& tIndices, const model_t& tModel, std::optional<mesh_index_t> tMeshIndex, uint32_t tMaxVertices, uint32_t tMaxIndices)>, decltype(aMeshletDivision)>) {
			generatedMeshlets = aMeshletDivision(aIndices, aModel.get(), aMeshIndex, aMaxVertices, aMaxIndices);
		}
		else if constexpr (std::is_assignable_v<std::function<std::vector<meshlet>(const std::vector<glm::vec3>& tVertices, const std::vector<uint32_t>& tIndices, const model_t& tModel, std::optional<mesh_index_t> tMeshIndex, uint32_t tMaxVertices, uint32_t tMaxIndices)>, decltype(aMeshletDivision)>) {
			generatedMeshlets = aMeshletDivision(aVertices, aIndices, aModel.get(), aMeshIndex, aMaxVertices, aMaxIndices);
		}
		else {
#if defined(_MSC_VER) && defined(__cplusplus)
			static_assert(false);
#else
			assert(false);
#endif
			throw avk::logic_error("No compatible lambda has been passed to divide_into_meshlets.");
		}

		for (auto& meshlet : generatedMeshlets)
		{
			meshlet.mModel = aModel;
		}

		return generatedMeshlets;
	}

	/** Divides the given models into meshlets using the default implementation divide_into_meshlets_simple.
	 *  @param	aModelsAndMeshletIndices				All the models and associated meshes that should be divided into meshlets.
	 *	@param	aCombineSubmeshes	If submeshes should be combined into a single vertex/index buffer.
	 *	@param	aMaxVertices		The maximum number of vertices of a meshlet.
	 *	@param	aMaxIndices			The maximum number of indices of a meshlet.
	 */
	std::vector<meshlet> divide_into_meshlets(std::vector<std::tuple<gvk::model, std::vector<mesh_index_t>>>& aModelsAndMeshletIndices,
		const bool aCombineSubmeshes = true, const uint32_t aMaxVertices = 64, const uint32_t aMaxIndices = 378);

	/** Divides the given models into meshlets using the given callback function.
	 *  @param	aModelsAndMeshletIndices				All the models and associated meshes that should be divided into meshlets.
	 *  @param	aMeshletDivision	Callback used to divide meshes into meshlets with a maximum number of vertices and indices each.
	 *								It can either receive the vertices and indices or just the indices depending on your specific needs.
	 *								Additionally it provides the model and an optional mesh index if more data is needed. If no mesh index is provided then the meshes were combined beforehand.
	 *								Ownership of the model must not be taken within the body of aMeshletDivision. The model will be assigned to each `meshlet` after aMeshletDivision has executed. \n\n
	 *								The callback must follow a specific declaration schema, optional parameters can be omitted, but all of them need to be provided in the following order:
	 *								 - const std::vector<glm::vec3>& tVertices:		optional	The vertices of the mesh or combined meshes of the model
	 *								 - const std::vector<uint32_t>& tIndices:  		mandatory	The indices of the mesh or combined meshes of the model
	 *								 - const model_t& tModel:						mandatory	The model these meshlets are generated from
	 *								 - std::optional<mesh_index_t> tMeshIndex: 		mandatory	The optional mesh index. If no value is provided, it means the meshes of the model are combined into a single vertex and index buffer
	 *								 - uint32_t tMaxVertices:						mandatory	The maximum number of vertices that are allowed in a single meshlet.
	 *								 - uint32_t tMaxIndices:						mandatory	The maximum number of indices that are allowed in a single meshlet.
	 *								Return value:
	 *								 - std::vector<meshlet>										The generated meshlets.\n\n
	 *								Example:
	 *								@code
	 *								[](const std::vector<glm::vec3>& tVertices, const std::vector<uint32_t>& aIndices,
	 *											const model_t& aModel, std::optional<mesh_index_t> aMeshIndex,
	 *											uint32_t aMaxVertices, uint32_t aMaxIndices) {
	 *									std::vector<gvk::meshlet> generatedMeshlets;
	 *									// Do your meshlet division here
	 *									return generatedMeshlets;
	 *								}
	 *								@endcode
	 *	@param	aCombineSubmeshes	If submeshes should be combined into a single vertex/index buffer.
	 *	@param	aMaxVertices		The maximum number of vertices of a meshlet. This value is just passed on to aMeshletDivision.
	 *	@param	aMaxIndices			The maximum number of indices of a meshlet. This value is just passed on to aMeshletDivision.
	 */
	template <typename F>
	std::vector<meshlet> divide_into_meshlets(std::vector<std::tuple<gvk::model, std::vector<gvk::mesh_index_t>>>& aModelsAndMeshletIndices, F aMeshletDivision,
		const bool aCombineSubmeshes = true, const uint32_t aMaxVertices = 64, const uint32_t aMaxIndices = 378)
	{
		std::vector<meshlet> meshlets;
		for (auto& pair : aModelsAndMeshletIndices) {
			auto& model = std::get<gvk::model>(pair);
			auto& meshIndices = std::get<std::vector<mesh_index_t>>(pair);

			if (aCombineSubmeshes) {
				auto [vertices, indices] = get_vertices_and_indices(make_model_references_and_mesh_indices_selection(model, meshIndices));
				std::vector<meshlet> tmpMeshlets = divide_indexed_geometry_into_meshlets(vertices, indices, std::move(model), std::nullopt, aMaxVertices, aMaxIndices, std::move(aMeshletDivision));
				// append to meshlets
				meshlets.insert(std::end(meshlets), std::make_move_iterator(std::begin(tmpMeshlets)), std::make_move_iterator(std::end(tmpMeshlets)));
			}
			else {
				for (const auto meshIndex : meshIndices) {
					auto vertices = model.get().positions_for_mesh(meshIndex);
					auto indices = model.get().indices_for_mesh<uint32_t>(meshIndex);
					std::vector<meshlet> tmpMeshlets = divide_indexed_geometry_into_meshlets(vertices, indices, std::move(model), meshIndex, aMaxVertices, aMaxIndices, std::move(aMeshletDivision));
					// append to meshlets
					meshlets.insert(std::end(meshlets), std::make_move_iterator(std::begin(tmpMeshlets)), std::make_move_iterator(std::end(tmpMeshlets)));
				}
			}
		}

		return meshlets;
	}
	
	// A concept which requires a type to have ::sNumVertices and ::sNumIndices
	template <typename T>
	concept has_static_num_vertices_and_num_indices = requires
	{
		T::sNumVertices; // Meaning: Static member T::sNumVertices exists
		T::sNumIndices;  // Meaning: Static member T::sNumIndices exists
	};
	
	/** Converts meshlets into a GPU usable representation.
	 *	@param	aMeshlets	The meshlets to convert
	 *	@tparam	T			Either meshlet_gpu_data or meshlet_indirect_gpu_data. If the indirect representation is used, the meshlet data will also be returned.
	 *						The meshlet data contains the vertex indices from [mDataOffset] to [mDataOffset + mVertexCount].
	 *						It also contains the indices into the vertex indices, four uint8 packed into a single uint32,
	 *						from [mDataOffset + mVertexCount] to [mDataOffset + mVertexCount + (mIndexCount+3)/4]
	 *  @tparam NV			The number of vertices
	 *  @tparam NI			The number of indices
	 *  @returns			A Tuple of the converted meshlets into the provided type and the optional meshlet data when the indirect representation
	 *						is used.
	 */
	template <typename T, size_t NV, size_t NI>
	std::tuple<std::vector<T>, std::optional<std::vector<uint32_t>>> convert_for_gpu_usage(const std::vector<meshlet>& aMeshlets)
	{
		std::vector<T> gpuMeshlets;
		std::optional<std::vector<uint32_t>> vertexIndices{};
		gpuMeshlets.reserve(aMeshlets.size());
		for (auto& meshlet : aMeshlets) {
			auto& newEntry = gpuMeshlets.emplace_back();
			if constexpr (std::is_convertible_v <T, meshlet_gpu_data <NV, NI>> ) {
				auto& ml = static_cast<meshlet_gpu_data<NV, NI>&>(newEntry);
				ml.mVertexCount = meshlet.mVertexCount;
				ml.mPrimitiveCount = meshlet.mIndexCount / 3u;
				std::ranges::copy(meshlet.mVertices, ml.mVertices);
				std::ranges::copy(meshlet.mIndices, ml.mIndices);
			}
			else if constexpr (std::is_convertible_v<T, meshlet_redirected_gpu_data>) {
				if (!vertexIndices.has_value()) {
					vertexIndices = std::vector<uint32_t>();
				}
				auto& ml = static_cast<meshlet_redirected_gpu_data&>(newEntry);
				ml.mVertexCount = meshlet.mVertexCount;
				ml.mPrimitiveCount = meshlet.mIndexCount / 3u;
				ml.mDataOffset = vertexIndices->size();
				// copy vertex indices
				vertexIndices->insert(vertexIndices->end(), meshlet.mVertices.begin(), meshlet.mVertices.end());
				// pack indices
				const uint32_t* indexGroups = reinterpret_cast<const uint32_t*>(meshlet.mIndices.data());
				const uint32_t indexGroupCount = (meshlet.mIndexCount + 3) / 4;
				vertexIndices->insert(vertexIndices->end(), indexGroups, indexGroups + indexGroupCount);
			}
			else {
#if defined(_MSC_VER) && defined(__cplusplus)
				static_assert(false);
#else
				assert(false);
#endif
				throw avk::logic_error("No suitable type passed to convert_for_gpu_usage.");
			}
		}
		return std::forward_as_tuple(gpuMeshlets, vertexIndices);
	}

	/** Converts meshlets into a GPU usable representation.
	 *	@param	aMeshlets	The meshlets to convert
	 *	@tparam	T			Either meshlet_gpu_data or meshlet_indirect_gpu_data. If the indirect representation is used, the meshlet data will also be returned.
	 *						The meshlet data contains the vertex indices from [mDataOffset] to [mDataOffset + mVertexCount].
	 *						It also contains the indices into the vertex indices, four uint8 packed into a single uint32,
	 *						from [mDataOffset + mVertexCount] to [mDataOffset + mVertexCount + (mIndexCount+3)/4]
	 *						T must provide static members ::sNumVertices and ::sNumIndices
	 *  @returns			A Tuple of the converted meshlets into the provided type and the optional meshlet data when the indirect representation
	 *						is used.
	 */
	template <typename T> requires has_static_num_vertices_and_num_indices<T>
	std::tuple<std::vector<T>, std::optional<std::vector<uint32_t>>> convert_for_gpu_usage(const std::vector<meshlet>& aMeshlets)
	{
		return convert_for_gpu_usage<T, T::sNumVertices, T::sNumIndices>(aMeshlets);
	}

	/** Converts meshlets into a GPU usable representation.
	 *  @param  aSerializer The serializer for the meshlet gpu data.
	 *	@param	aMeshlets	The meshlets to convert
	 *	@tparam	T			Either meshlet_gpu_data or meshlet_indirect_gpu_data. If the indirect representation is used, the meshlet data will also be returned.
	 *						The meshlet data contains the vertex indices from [mDataOffset] to [mDataOffset + mVertexCount].
	 *						It also contains the indices into the vertex indices, four uint8 packed into a single uint32,
	 *						from [mDataOffset + mVertexCount] to [mDataOffset + mVertexCount + (mIndexCount+3)/4]
	 *  @tparam NV			The number of vertices
	 *	@tparam NI			The number of indices
	 *  @returns			A Tuple of the converted meshlets into the provided type and the optional meshlet data when the indirect representation
	 *						is used.
	 */
	template <typename T, size_t NV, size_t NI>
	std::tuple<std::vector<T>, std::optional<std::vector<uint32_t>>> convert_for_gpu_usage_cached(serializer& aSerializer, const std::vector<meshlet>& aMeshlets)
	{
		std::tuple<std::vector<T>, std::optional<std::vector<uint32_t>>> result;
		auto& resultMeshlets = std::get<std::vector<T>>(result);
		auto& resultMeshletsData = std::get<std::optional<std::vector<uint32_t>>>(result);
		if (aSerializer.mode() == serializer::mode::serialize)
		{
			std::tie(resultMeshlets, resultMeshletsData) = convert_for_gpu_usage<T, NV, NI>(aMeshlets);
		}

		if constexpr (std::is_convertible_v<T, meshlet_gpu_data<NV, NI>>)
		{
			aSerializer.archive(resultMeshlets);
		}
		else if constexpr (std::is_convertible_v<T, meshlet_redirected_gpu_data>)
		{
			aSerializer.archive(resultMeshlets);
			aSerializer.archive(resultMeshletsData);
		}
		else {
#if defined(_MSC_VER) && defined(__cplusplus)
			static_assert(false);
#else
			assert(false);
#endif
			throw avk::logic_error("No suitable type passed to convert_for_gpu_usage_cached.");
		}
		return result;
	}

	/** Converts meshlets into a GPU usable representation.
	 *  @param  aSerializer The serializer for the meshlet gpu data.
	 *	@param	aMeshlets	The meshlets to convert
	 *	@tparam	T			Either meshlet_gpu_data or meshlet_indirect_gpu_data. If the indirect representation is used, the meshlet data will also be returned.
	 *						The meshlet data contains the vertex indices from [mDataOffset] to [mDataOffset + mVertexCount].
	 *						It also contains the indices into the vertex indices, four uint8 packed into a single uint32,
	 *						from [mDataOffset + mVertexCount] to [mDataOffset + mVertexCount + (mIndexCount+3)/4].
	 *						T must provide static members ::sNumVertices and ::sNumIndices
	 *  @returns			A Tuple of the converted meshlets into the provided type and the optional meshlet data when the indirect representation
	 *						is used.
	 */
	template <typename T> requires has_static_num_vertices_and_num_indices<T>
	std::tuple<std::vector<T>, std::optional<std::vector<uint32_t>>> convert_for_gpu_usage_cached(serializer& aSerializer, const std::vector<meshlet>& aMeshlets)
	{
		return convert_for_gpu_usage_cached<T, T::sNumVertices, T::sNumIndices>(aSerializer, aMeshlets);
	}

}
