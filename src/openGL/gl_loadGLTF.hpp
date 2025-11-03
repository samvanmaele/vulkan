#pragma once
#include <tinygltf/tiny_gltf.h>
#include <glm/glm.hpp>
//#include <string>
//#include <optional>

#ifdef __EMSCRIPTEN__
	#include <emscripten.h>
	#include <emscripten/html5.h>
	#include <GLES3/gl3.h>
#else
	#include <GL/glew.h>
#endif

/*
struct rayHit
{
	float distance;
	glm::vec3 normal;
};
struct boundingbox
{
	glm::vec3 min;
	glm::vec3 max;

	std::optional<rayHit> intersectRay(const glm::vec3& ray, const glm::vec3& origin);
};
inline std::optional<rayHit> boundingbox::intersectRay(const glm::vec3& ray, const glm::vec3& origin)
{
	glm::vec3 dirFrac = 1.0f / ray;

	float t1 = (min.x - origin.x) * dirFrac.x;
	float t2 = (max.x - origin.x) * dirFrac.x;
	float t3 = (min.y - origin.y) * dirFrac.y;
	float t4 = (max.y - origin.y) * dirFrac.y;
	float t5 = (min.z - origin.z) * dirFrac.z;
	float t6 = (max.z - origin.z) * dirFrac.z;

	float tmin = std::max({std::min(t1, t2), std::min(t3, t4), std::min(t5, t6)});
	float tmax = std::min({std::max(t1, t2), std::max(t3, t4), std::max(t5, t6)});

	if (tmax < 0 || tmin > tmax) {return std::nullopt;}
	float t = (tmin >= 0) ? tmin : tmax;

	glm::vec3 normal(0.0f);
	if (t == t1 || t == t2) {normal.x = (t == t1) ? -1.0f : 1.0f;}
    else if (t == t3 || t == t4) {normal.y = (t == t3) ? -1.0f : 1.0f;}
    else if (t == t5 || t == t6) {normal.z = (t == t5) ? -1.0f : 1.0f;}

	return rayHit{t, normal};
}
*/

class GlModel
{
	public:
		struct PrimitiveData
		{
			GLuint vao;
			GLsizei count;
			GLenum indexType;
			GLenum mode;
			uint32_t textureIndex = -1;
		};
		std::vector<PrimitiveData> primitiveDataList;

		glm::mat4 transmat = glm::mat4(1.0);

		//std::vector<boundingbox> boundingboxes;
		//boundingbox aabb;

		GlModel() = default;
		GlModel(const char* filename);
		GlModel(GlModel&& other) noexcept
		{
			primitiveDataList = std::move(other.primitiveDataList);
			transmat[0] = other.transmat[0];
			transmat[1] = other.transmat[1];
			transmat[2] = other.transmat[2];
			textureMap = std::move(other.textureMap);

			other.primitiveDataList.clear();
			other.textureMap.clear();
		}

		GlModel& operator=(GlModel&& other) noexcept
		{
			if (this != &other)
			{
				primitiveDataList = std::move(other.primitiveDataList);
				transmat[0] = other.transmat[0];
				transmat[1] = other.transmat[1];
				transmat[2] = other.transmat[2];
				textureMap = std::move(other.textureMap);

				other.primitiveDataList.clear();
				other.textureMap.clear();
			}
			return *this;
		}

		void drawModel();
		void drawDepth();
		~GlModel();

	private:
		std::unordered_map<int, GLuint> textureMap;

		void bindNode(tinygltf::Model& model, const tinygltf::Node& node);
		void bindMesh(tinygltf::Model& model, const tinygltf::Node& node);
		void bindPos(tinygltf::Model& model, int binding, int vecSize, int attribPos, glm::mat4 nodeMatrix);
		void bindAttrib(tinygltf::Model& model, int binding, int vecSize, int attribPos);
		void createTexture(const tinygltf::Image& image, int index);
};