#pragma once
#include "HLSLCompat.h"
#include <vector>

typedef struct Mesh
{
	std::vector<Vertex> vertices;
	std::vector<Index> indices;
};

typedef struct Model
{
	std::vector<Mesh> meshes;
};