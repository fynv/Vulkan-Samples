#include "animation.h"

namespace vkb
{
namespace sg
{
glm::vec4 AnimationSampler::cubicSplineInterpolation(size_t index, float time, uint32_t stride)
{
	// Details on how this works can be found in the specs:
	// https://github.com/KhronosGroup/glTF/tree/master/specification/2.0#appendix-c-spline-interpolation
	auto           delta   = inputs[index + 1] - inputs[index];
	auto           t       = (time - inputs[index]) / delta;
	const uint32_t current = static_cast<uint32_t>(index * stride * 3);
	const uint32_t next    = static_cast<uint32_t>((index + 1) * stride * 3);
	const uint32_t A       = 0;
	const uint32_t V       = stride * 1;
	const uint32_t B       = stride * 2;

	float     t2 = pow(t, 2);
	float     t3 = pow(t, 3);
	glm::vec4 pt;
	for (uint32_t i = 0; i < stride; i++)
	{
		float p0 = outputs[current + i + V];                // starting point at t = 0
		float m0 = delta * outputs[current + i + A];        // scaled starting tangent at t = 0
		float p1 = outputs[next + i + V];                   // ending point at t = 1
		float m1 = delta * outputs[next + i + B];           // scaled ending tangent at t = 1
		pt[i]    = ((2.0f * t3 - 3.0f * t2 + 1.0f) * p0) + ((t3 - 2.0f * t2 + t) * m0) + ((-2.0f * t3 + 3.0f * t2) * p1) + ((t3 - t2) * m0);
	}
	return pt;
}

void AnimationSampler::translate(size_t index, float time, sg::Node &node)
{
	auto &transform = node.get_transform();
	switch (interpolation)
	{
		case AnimationSampler::InterpolationType::LINEAR:
		{
			float u = std::max(0.0f, time - inputs[index]) / (inputs[index + 1] - inputs[index]);
			transform.set_translation(glm::mix(outputsVec4[index], outputsVec4[index + 1], u));
			break;
		}
		case AnimationSampler::InterpolationType::STEP:
		{
			transform.set_translation(outputsVec4[index]);
			break;
		}
		case AnimationSampler::InterpolationType::CUBICSPLINE:
		{
			transform.set_translation(cubicSplineInterpolation(index, time, 3));
			break;
		}
	}
}

void AnimationSampler::scale(size_t index, float time, sg::Node &node)
{
	auto &transform = node.get_transform();
	switch (interpolation)
	{
		case AnimationSampler::InterpolationType::LINEAR:
		{
			float u = std::max(0.0f, time - inputs[index]) / (inputs[index + 1] - inputs[index]);
			transform.set_scale(glm::mix(outputsVec4[index], outputsVec4[index + 1], u));
			break;
		}
		case AnimationSampler::InterpolationType::STEP:
		{
			transform.set_scale(outputsVec4[index]);
			break;
		}
		case AnimationSampler::InterpolationType::CUBICSPLINE:
		{
			transform.set_scale(cubicSplineInterpolation(index, time, 3));
			break;
		}
	}
}

void AnimationSampler::rotate(size_t index, float time, sg::Node &node)
{
	auto &transform = node.get_transform();
	switch (interpolation)
	{
		case AnimationSampler::InterpolationType::LINEAR:
		{
			float     u = std::max(0.0f, time - inputs[index]) / (inputs[index + 1] - inputs[index]);
			glm::quat q1;
			q1.x = outputsVec4[index].x;
			q1.y = outputsVec4[index].y;
			q1.z = outputsVec4[index].z;
			q1.w = outputsVec4[index].w;
			glm::quat q2;
			q2.x = outputsVec4[index + 1].x;
			q2.y = outputsVec4[index + 1].y;
			q2.z = outputsVec4[index + 1].z;
			q2.w = outputsVec4[index + 1].w;
			// @todo: only takes in quaternions
			transform.set_rotation(glm::normalize(glm::slerp(q1, q2, u)));
			break;
		}
		case AnimationSampler::InterpolationType::STEP:
		{
			glm::quat q1;
			q1.x = outputsVec4[index].x;
			q1.y = outputsVec4[index].y;
			q1.z = outputsVec4[index].z;
			q1.w = outputsVec4[index].w;
			transform.set_rotation(q1);
			break;
		}
		case AnimationSampler::InterpolationType::CUBICSPLINE:
		{
			glm::vec4 rot = cubicSplineInterpolation(index, time, 4);
			glm::quat q;
			q.x = rot.x;
			q.y = rot.y;
			q.z = rot.z;
			q.w = rot.w;
			transform.set_rotation(glm::normalize(q));
			break;
		}
	}
}

Animation::Animation(const std::string &name) :
    Component{name}
{}

std::type_index Animation::get_type()
{
	return typeid(Animation);
}
void Animation::update(float delta_time)
{
	bool updated = false;
	for (auto &animation_channel : animation_channels)
	{
		sg::AnimationSampler &animation_sampler = animation_samplers[animation_channel.samplerIndex];
		if (animation_sampler.inputs.size() > animation_sampler.outputsVec4.size())
		{
			continue;
		}

		for (size_t i = 0; i < animation_sampler.inputs.size() - 1; i++)
		{
			if ((current_time >= animation_sampler.inputs[i]) && (current_time <= animation_sampler.inputs[i + 1]))
			{
				float u = std::max(0.0f, current_time - animation_sampler.inputs[i]) / (animation_sampler.inputs[i + 1] - animation_sampler.inputs[i]);
				if (u <= 1.0f)
				{
					switch (animation_channel.path)
					{
						case sg::AnimationChannel::PathType::TRANSLATION:
							animation_sampler.translate(i, current_time, *animation_channel.node);
							break;
						case sg::AnimationChannel::PathType::SCALE:
							animation_sampler.scale(i, current_time, *animation_channel.node);
							break;
						case sg::AnimationChannel::PathType::ROTATION:
							animation_sampler.rotate(i, current_time, *animation_channel.node);
					}
					updated = true;
				}
			}
		}
	}
	current_time += delta_time;
	if (current_time > end)
	{
		// @todo: Proper wrapping
		current_time = 0.0f;
	}
}
}        // namespace sg
}        // namespace vkb