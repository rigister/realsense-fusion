
#include "icp.h"
#include "frame.h"
#include "camera_transform.h"
#include "shader_common.h"

#define RESIDUAL_COMPONENTS 6

static const char *corr_shader_code =
"#version 450 core\n"
#include "glsl_common_projection.inl"
R"glsl(

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

uniform float depth_scale;

layout(binding = 0) uniform sampler2D vertex_tex_current;
layout(binding = 1) uniform sampler2D normal_tex_current;

layout(std430, binding = 0) buffer residuals_out
{
	float residuals[];
};

ivec2 coord;

void StoreResidual(vec3 vertex, vec3 normal)
{
	uint base = 6 * uint(coord.y * camera_intrinsics.res.x + coord.x);
	residuals[base+0] = vertex.x;
	residuals[base+1] = vertex.y;
	residuals[base+2] = vertex.z;
	residuals[base+3] = normal.x;
	residuals[base+4] = normal.y;
	residuals[base+5] = normal.z;
}

void StoreNopResidual()
{
	StoreResidual(vec3(0.0), vec3(0.0));
}

void main()
{
	coord = ivec2(gl_GlobalInvocationID.xy);
	vec3 vertex_current = texelFetch(vertex_tex_current, coord, 0).xyz;
	if(isinf(vertex_current.x))
	{
		StoreNopResidual();
		return;
	}
	vec3 normal_current = texelFetch(normal_tex_current, coord, 0).xyz;
	StoreResidual(vertex_current, normal_current);
}
)glsl";

ICP::ICP()
{
	corr_program = CreateComputeShader(corr_shader_code);
	glObjectLabel(GL_PROGRAM, corr_program, -1, "ICP::corr_program");

	glGenBuffers(1, &residuals_buffer);
	glObjectLabel(GL_BUFFER, residuals_buffer, -1, "ICP::residuals_buffer");
	residuals_buffer_size = 0;
}

ICP::~ICP()
{
	glDeleteProgram(corr_program);
	glDeleteBuffers(1, &residuals_buffer);
}

void ICP::SearchCorrespondences(Frame *frame, const CameraTransform &cam_transform_old, CameraTransform *cam_transform_new)
{
	auto residuals_buffer_size_required =
			static_cast<size_t>(frame->GetDepthWidth() * frame->GetDepthHeight())
			* RESIDUAL_COMPONENTS * sizeof(float);
	if (residuals_buffer_size_required != residuals_buffer_size)
	{
		residuals_buffer_size = residuals_buffer_size_required;
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, residuals_buffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, residuals_buffer_size, nullptr, GL_DYNAMIC_COPY);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, residuals_buffer);

	glUseProgram(corr_program);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, frame->GetVertexTex());
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, frame->GetNormalTex());

	glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
	glDispatchCompute(static_cast<GLuint>(frame->GetDepthWidth()), static_cast<GLuint>(frame->GetDepthHeight()), 1);
}