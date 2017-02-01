#pragma once

#include <iostream>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/quaternion.hpp>

/// <summary> Represents a transform: rotation, position and scale. </summary>
class Transform {
public:
	glm::vec3 position = { 0,0,0 }, scale = { 1,1,1 }, rotation = { 0,0,0 };
	Transform * parent = nullptr;

	Transform();

	/// <summary> Is true when the transform matrix is not correctly representing the position, scale and rotation vectors. </summary>
	bool transformIsInvalid = false;

	/// <summary> Recalculates the transform matrix according to the position, scale and rotation vectors. </summary>
	void updateTransformMatrix();

	/// <summary> Returns a reference to the transform matrix </summary>
	glm::mat4 & getTransformMatrix();

	/// <summary> Output. </summary>
	friend std::ostream & operator<<(std::ostream &, const Transform &);

	// Bunch of helper functions.
	glm::vec3 forward();
	glm::vec3 up();
	glm::vec3 right();
private:
	glm::mat4 transform;
};
