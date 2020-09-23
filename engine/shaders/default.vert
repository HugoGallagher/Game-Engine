#version 450

const int MAX_MATERIALS = 255;
const int MAX_MESHES = 255;

struct material
{
	vec3 colour;
};
struct mesh_data
{
	vec3 model;
};

layout(location=0) in vec3 in_pos;
layout(location=1) in int mat_i;

layout(location=0) out vec3 f_col;

layout(set=0,binding=0) uniform materials
{
	material mats[MAX_MATERIALS];
};
layout(set=1,binding=1) uniform mesh_transforms
{
	mesh_data meshes[MAX_MESHES];
};

void main()
{
	int index = gl_InstanceIndex;
	//int index = 0;

	//gl_Position = vec4(in_pos, 1.0);
	gl_Position = vec4(meshes[index].model + in_pos, 1.0);
	//f_col = vec3(meshes[index].model.x, 0, 0);
	f_col = mats[index].colour;
}