#pragma once
#include <string>
#include <vector>
#include <fstream>

#include "DMGraphicsPrimitives.h"
#include "DMMeshRenderable.h"

namespace dm::core::utility
{
	inline std::vector<uint8_t> ReadBinaryFile(const std::string& path)
	{
		std::ifstream stream(path, std::ios::binary);

		std::vector<uint8_t> rawFile((std::istreambuf_iterator(stream)), std::istreambuf_iterator<char>());

		return rawFile;
	}

	inline std::vector<char> StringToCharBufferCopy(const std::string& str)
	{
		std::vector<char> buffer;

		for (auto& c : str)
		{
			buffer.push_back(c);
		}
	}

    std::wstring ToWideString(const std::string& input);

    inline void SubdivideGrid_Internal(int edges, dm::core::MeshRenderable& renderable)
    {
        // The half-length is 128 / 2 = 64 units.
        const float halfLength = 64.0f;

        // Define the four vertices of the quad.
        DMVertex quadVertices[4] = {
            // Bottom-right vertex
            { glm::vec3(halfLength, 0.0f, -halfLength), glm::vec2(1.0f, 0.0f) },
            // Bottom-left vertex
            { glm::vec3(-halfLength, 0.0f, -halfLength), glm::vec2(0.0f, 0.0f) },

            // Top-left vertex
            { glm::vec3(-halfLength, 0.0f,  halfLength), glm::vec2(0.0f, 1.0f) },
            // Top-right vertex
            { glm::vec3(halfLength, 0.0f,  halfLength), glm::vec2(1.0f, 1.0f) },
        };

        const int subdivisions = edges;
        const int vertexCountPerSide = subdivisions + 1;
        const float skirtHeight = 50.0f;

        renderable.vertices.clear();
        renderable.indices.clear();

        // Retrieve the cell's corner positions.
        glm::vec3 topLeft = quadVertices[3].position;
        glm::vec3 topRight = quadVertices[2].position;
        glm::vec3 bottomLeft = quadVertices[0].position;
        glm::vec3 bottomRight = quadVertices[1].position;

        // Retrieve the cell's UV coordinates.
        glm::vec2 uvTopLeft = quadVertices[3].uvChannel1;
        glm::vec2 uvTopRight = quadVertices[2].uvChannel1;
        glm::vec2 uvBottomLeft = quadVertices[0].uvChannel1;
        glm::vec2 uvBottomRight = quadVertices[1].uvChannel1;

        // Generate vertices for the main grid.
        for (int j = 0; j < vertexCountPerSide; ++j)
        {
            float v = static_cast<float>(j) / subdivisions;
            for (int i = 0; i < vertexCountPerSide; ++i)
            {
                float u = static_cast<float>(i) / subdivisions;
                // Bilinear interpolation for position.
                glm::vec3 pos =
                    (1.0f - u) * (1.0f - v) * topLeft +
                    u * (1.0f - v) * topRight +
                    (1.0f - u) * v * bottomLeft +
                    u * v * bottomRight;

                // Bilinear interpolation for UV coordinates.
                glm::vec2 uv =
                    (1.0f - u) * (1.0f - v) * uvTopLeft +
                    u * (1.0f - v) * uvTopRight +
                    (1.0f - u) * v * uvBottomLeft +
                    u * v * uvBottomRight;

                renderable.vertices.push_back({ .position = pos, .uvChannel1 = uv, .flags = DMVertex_Flag_None });
            }
        }

        // Generate indices for the main grid (two triangles per quad).
        for (int j = 0; j < subdivisions; ++j)
        {
            for (int i = 0; i < subdivisions; ++i)
            {
                int topLeftIndex = j * vertexCountPerSide + i;
                int topRightIndex = topLeftIndex + 1;
                int bottomLeftIndex = (j + 1) * vertexCountPerSide + i;
                int bottomRightIndex = bottomLeftIndex + 1;

                // Triangle 1
                renderable.indices.push_back(topLeftIndex);
                renderable.indices.push_back(topRightIndex);
                renderable.indices.push_back(bottomLeftIndex);

                // Triangle 2
                renderable.indices.push_back(topRightIndex);
                renderable.indices.push_back(bottomRightIndex);
                renderable.indices.push_back(bottomLeftIndex);
            }
        }

        // ===== Add Skirts =====

        // --- Top Edge Skirt (row 0) ---
        int topSkirtStart = static_cast<int>(renderable.vertices.size());
        for (int i = 0; i < vertexCountPerSide; ++i)
        {
            int mainIndex = i;  // row 0 has indices 0 .. vertexCountPerSide-1
            auto vertex = renderable.vertices[mainIndex];
            vertex.position.y -= skirtHeight;  // Lower the vertex to create the skirt
            vertex.flags = DMVertex_Flag_NoHeightMap;
            renderable.vertices.push_back(vertex);
        }
        // Connect the top edge.
        for (int i = 0; i < vertexCountPerSide - 1; ++i)
        {
            int mainLeft = 0 + i;
            int mainRight = 0 + (i + 1);
            int skirtLeft = topSkirtStart + i;
            int skirtRight = topSkirtStart + (i + 1);

            renderable.indices.push_back(mainLeft);
            renderable.indices.push_back(skirtRight);
            renderable.indices.push_back(mainRight);

            renderable.indices.push_back(mainLeft);
            renderable.indices.push_back(skirtLeft);
            renderable.indices.push_back(skirtRight);
        }

        // --- Bottom Edge Skirt (last row) ---
        int bottomRow = vertexCountPerSide - 1;
        int bottomSkirtStart = static_cast<int>(renderable.vertices.size());
        for (int i = 0; i < vertexCountPerSide; ++i)
        {
            int mainIndex = (bottomRow * vertexCountPerSide + i);
            auto vertex = renderable.vertices[mainIndex];
            vertex.position.y -= skirtHeight;
            vertex.flags = DMVertex_Flag_NoHeightMap;
            renderable.vertices.push_back(vertex);
        }
        // Connect the bottom edge.
        for (int i = 0; i < vertexCountPerSide - 1; ++i)
        {
            int mainLeft = bottomRow * vertexCountPerSide + i;
            int mainRight = bottomRow * vertexCountPerSide + (i + 1);
            int skirtLeft = bottomSkirtStart + i;
            int skirtRight = bottomSkirtStart + (i + 1);
            // The winding order here is adjusted to maintain consistency.
            renderable.indices.push_back(mainLeft);
            renderable.indices.push_back(mainRight);
            renderable.indices.push_back(skirtRight);

            renderable.indices.push_back(mainLeft);
            renderable.indices.push_back(skirtRight);
            renderable.indices.push_back(skirtLeft);
        }

        // --- Left Edge Skirt (column 0) ---
        int leftSkirtStart = static_cast<int>(renderable.vertices.size());
        for (int j = 0; j < vertexCountPerSide; ++j)
        {
            int mainIndex = (j * vertexCountPerSide);
            auto vertex = renderable.vertices[mainIndex];
            vertex.position.y -= skirtHeight;
            vertex.flags = DMVertex_Flag_NoHeightMap;
            renderable.vertices.push_back(vertex);
        }
        // Connect the left edge.
        for (int j = 0; j < vertexCountPerSide - 1; ++j)
        {
            int mainTop = j * vertexCountPerSide + 0;
            int mainBottom = (j + 1) * vertexCountPerSide + 0;
            int skirtTop = leftSkirtStart + j;
            int skirtBottom = leftSkirtStart + (j + 1);

            renderable.indices.push_back(mainTop);
            renderable.indices.push_back(skirtBottom);
            renderable.indices.push_back(skirtTop);

            renderable.indices.push_back(mainTop);
            renderable.indices.push_back(mainBottom);
            renderable.indices.push_back(skirtBottom);
        }

        // --- Right Edge Skirt (last column) ---
        int rightColumn = vertexCountPerSide - 1;
        int rightSkirtStart = static_cast<int>(renderable.vertices.size());
        for (int j = 0; j < vertexCountPerSide; ++j)
        {
            int mainIndex = (j * vertexCountPerSide + rightColumn);
            auto vertex = renderable.vertices[mainIndex];
            vertex.position.y -= skirtHeight;
            vertex.flags = DMVertex_Flag_NoHeightMap;
            renderable.vertices.push_back(vertex);
        }
        // Connect the right edge.
        for (int j = 0; j < vertexCountPerSide - 1; ++j)
        {
            int mainTop = j * vertexCountPerSide + rightColumn;
            int mainBottom = (j + 1) * vertexCountPerSide + rightColumn;
            int skirtTop = rightSkirtStart + j;
            int skirtBottom = rightSkirtStart + (j + 1);

            renderable.indices.push_back(mainTop);
            renderable.indices.push_back(skirtBottom);
            renderable.indices.push_back(mainBottom);

            renderable.indices.push_back(mainTop);
            renderable.indices.push_back(skirtTop);
            renderable.indices.push_back(skirtBottom);
        }
    }

}
