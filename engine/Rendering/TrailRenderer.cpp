#include "TrailRenderer.h"
#include "Camera.h"
#include "TextureManager.h"

#include <iostream>

#define attribOffset(field) (reinterpret_cast<void *>(offsetof(TrailMesh::MeshDataType, field)))
constexpr int VerticesPerSegment = 2;

void bindAttribute(GLuint location, size_t size, int type, bool doNormalize, void* offset)
{
    glVertexAttribPointer(location, size, type, doNormalize ? GL_TRUE : GL_FALSE, sizeof(TrailMesh::MeshDataType), offset);
    glEnableVertexAttribArray(location);
}

TrailMesh::TrailMesh(size_t size):
	vbo(0)
{
	using namespace glm;
	data.resize(size); // make mesh data buffer its fixed size
	
	// 1. create vao
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// 2. create vbo for vertex data
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	// 3. fill data
	glBufferData(GL_ARRAY_BUFFER, sizeof(MeshDataType) * data.size(), &data[0], GL_DYNAMIC_DRAW);

	// 4. bind attributes
    bindAttribute(0, 3, GL_FLOAT, false, attribOffset(position));
    bindAttribute(1, 4, GL_FLOAT, false, attribOffset(color));
    bindAttribute(2, 2, GL_FLOAT, true, attribOffset(uv));
    
	glBindVertexArray(0);
    std::cout << "mesh vao: " << vao << " created" << std::endl;
}

TrailMesh::~TrailMesh()
{
	std::cout << "mesh vao deleted: " << vao << std::endl;
	glDeleteVertexArrays(1, &vao);
}

TrailRenderer::TrailRenderer(GameObject* _gameObject,
	int _segmentsCount,
	float _trailWidth, 	const std::string& textureFileName):
	Renderer(createMesh(_segmentsCount)), // triangle strip mesh
	gameObject(_gameObject),
	maxSegmentsCount(_segmentsCount),
	usedSegmentsCount(0),
	trailWidth(_trailWidth),
	texture(TextureManager::getTexture(textureFileName))
{
	segments.resize(_segmentsCount);
	for (auto& s : segments)
		s = glm::vec3(0.0f, 0.0f, 0.0f);
}

TrailRenderer::~TrailRenderer()
{
}

void TrailRenderer::render(const Camera* const camera) const {
	// there must be at least 2 segments to get something drawable
	if (usedSegmentsCount < 2)
		return;

	// 1. bind skybox vao
	glBindVertexArray(mesh->vao);

	// 2. use skybox program
	glUseProgram(shaderProgram->programId());

	// 2.1 draw triangle strip
    // draw line for a while
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	// 2.2 Blending
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// 3. set active textures
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture->id);

	// 4. fill uniform variables
    glm::mat4 viewProjectionMatrix = camera->getViewProjectionMatrix();
	rudy::setUniformMat4(shaderProgram->programId(), "ViewProjection", viewProjectionMatrix);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, usedSegmentsCount * VerticesPerSegment);
    
	// 5. disable blending and polygon modes
	glDisable(GL_BLEND);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

std::shared_ptr<TrailMesh> TrailRenderer::createMesh(size_t segmentsCount) {
	auto result = std::make_shared<TrailMesh>(segmentsCount * VerticesPerSegment);
	return result;
}

void TrailRenderer::update() {
	using namespace glm;
	
    // 1. Get current gameObject position
	auto objPos = gameObject->transform->getPosition();

	// 2. Compare with previously stored position (emit new segment)
	if (usedSegmentsCount < 2 || length(objPos - segments[1]) > trailWidth) {
		usedSegmentsCount = min(++usedSegmentsCount, maxSegmentsCount);
		// 2.1 shift positions and forget last
		for (int i = segments.size() - 1; i > 0; --i)
			segments[i] = segments[i - 1];
	}

	// 3. First segment is always sticked to gameObject
	segments[0] = objPos;
    
	// 4. fade out trail
	if (usedSegmentsCount > 1) {
		auto& lastSegmentPos = segments[usedSegmentsCount - 1];
		auto preLastSegmentPos = segments[usedSegmentsCount - 2];

		auto lastSegmentVector = preLastSegmentPos - lastSegmentPos;
		float segmentLength = length(lastSegmentVector);
		constexpr float fadeOutSpeed = 0.0002f;
		if(segmentLength < fadeOutSpeed) {
			--usedSegmentsCount;
		}
		else {
			lastSegmentPos += normalize(lastSegmentVector) * fadeOutSpeed;
		}
	}
    
    // 5. make smoother angles
    /*constexpr float smoothFactor = 0.01f;
    
    for(int i = 0; i < usedSegmentsCount - 2 && i < segments.size() - 2; ++i) {
        auto& first = segments[i];
        auto& second = segments[i + 1];
        auto& third = segments[i + 2];
        
        auto middle = (first + third) * 0.5f;
        second = (1 - smoothFactor) * second + smoothFactor * middle;
    }*/
	
    // 6. Recalculate vbo data
    updateMeshData();
}


/*
 todo: This logic can be implemented on GPU side with Vertex shader with instance data array as for particles
 */
void TrailRenderer::updateMeshData() {
	using namespace glm;

	// fill positions
	for (int i = 0; i < segments.size() && i < usedSegmentsCount; ++i) {
		bool isLastSegment = i == segments.size() - 1;
		auto point = segments[i];
		auto nextPoint = segments[isLastSegment ? i - 1 : i + 1]; // take prev point for last to get direction

		auto deltaVector = point - nextPoint;
        
		float factor = ((segments.size() - i) / static_cast<float>(segments.size()));
		float segmentWidth = trailWidth * factor; // narrower at the end of trail

		auto Matrix = glm::mat3_cast(gameObject->transform->getRotation());
		
		//auto p1 = point + Matrix * normalize(vec3(-deltaVector.y, deltaVector.x, point.z)) * segmentWidth;
		//auto p2 = point + Matrix * normalize(vec3(deltaVector.y, -deltaVector.x, nextPoint.z)) * segmentWidth;

		vec3 up = Matrix * vec3(0.0f, 0.9f, 0.1f); // WTF? Doesn't work with (0, 1, 0) ???
		vec3 dNormalized = deltaVector;
		vec3 crossResult = glm::cross(dNormalized, up);

		//std::cout << "cross (" << dNormalized << ", " << up << ") = " << crossResult << std::endl;

		auto orthogonal = normalize(crossResult);
		auto p1 = point + orthogonal * segmentWidth;
		auto p2 = point - orthogonal * segmentWidth;

		//std::cout << "width " << segmentWidth << ", " << length(p1 - p2) << std::endl;

		size_t meshIndex = VerticesPerSegment * i;
		float uvOffset = i % 2 == 0 ? 1 : 0;

		// At the last segment vertices must be flipped
		// to get nicely closed last segment 'ribbon'
		auto& left  = isLastSegment ? mesh->data[meshIndex] : mesh->data[meshIndex + 1];
		auto& right = isLastSegment ? mesh->data[meshIndex + 1] : mesh->data[meshIndex];
		vec2 uv1 = isLastSegment ? vec2(0, uvOffset) : vec2(1, uvOffset);
		vec2 uv2 = isLastSegment ? vec2(1, uvOffset) : vec2(0, uvOffset);

		left.position = p1;
		left.color = vec4(65, 204, 242, factor);
		left.uv = uv1;

        right.position = p2;
		right.uv = uv2;
		right.color = vec4(65, 204, 242, factor);
	}
    
 	glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(VertexDataType) * mesh->data.size(), &mesh->data[0]);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}
