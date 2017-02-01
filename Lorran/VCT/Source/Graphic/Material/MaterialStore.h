#pragma once

#include <iostream>
#include <vector>

#include "Material.h"

//class Material;

/// <summary> Manages all loaded materials and shader programs. </summary>
class MaterialStore {
public:
	static MaterialStore & getInstance();
	std::vector<Material*> materials;
	Material * findMaterialWithName(std::string name);
	Material * findMaterialWithProgramID(unsigned int programID);
	void AddNewMaterial(
		std::string name, const char * vertexPath = nullptr, const char * fragmentPath = nullptr,
		const char * geometryPath = nullptr, const char * tessEvalPath = nullptr, const char * tessCtrlPath = nullptr);
	~MaterialStore();
	MaterialStore(MaterialStore const &) = delete;
	void operator=(MaterialStore const &) = delete;
private:
	MaterialStore();
};
