#pragma once

#include <limits>
#include <vector>

#include "scene_graph/component.h"

namespace vkb
{
namespace sg
{
class AnimationChannel
{
  public:
	enum PathType
	{
		TRANSLATION,
		ROTATION,
		SCALE
	};
	PathType path;
	uint32_t samplerIndex;
	sg::Node *node;
};
class AnimationSampler
{
  private:
	glm::vec4 cubicSplineInterpolation(size_t index, float time, uint32_t stride);

  public:
	enum InterpolationType
	{
		LINEAR,
		STEP,
		CUBICSPLINE
	};
	InterpolationType      interpolation;
	std::vector<float>     inputs;
	std::vector<glm::vec4> outputsVec4;
	std::vector<float>     outputs;
	void                   translate(size_t index, float time, sg::Node &node);
	void                   scale(size_t index, float time, sg::Node &node);
	void                   rotate(size_t index, float time, sg::Node &node);
};
class Animation : public Component
{
  public:
	Animation(const std::string &name);

	Animation(Animation &&other) = default;

	virtual ~Animation() = default;

	virtual std::type_index get_type() override;

	void update(float delta_time);

	float current_time = 0.0f;

	std::vector<AnimationSampler> animation_samplers;
	std::vector<AnimationChannel> animation_channels;

	float start = std::numeric_limits<float>::max();
	float end   = std::numeric_limits<float>::min();
};
}        // namespace sg
}        // namespace vkb