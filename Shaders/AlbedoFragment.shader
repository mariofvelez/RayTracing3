#version 430 core

out vec4 FragColor;

in vec2 TexCoord;

uniform mat4 camera;
uniform vec3 camera_pos;

uniform int num_nodes;
uniform int curr_frame;
uniform sampler2D dragon_texture;
uniform sampler2D skybox_texture;
const float pi = 3.14189265;

// structs
struct Material
{
	vec3 albedo;
};

struct Primitive
{
	uint vertex_a;
	uint vertex_b;
	uint vertex_c;

	uint normal_a;
	uint normal_b;
	uint normal_c;

	uint material;
};

struct Node
{
	int axis;
	int left;
	int num_tris;
	int tri_index;
	vec3 min;
	vec3 max;
};

struct Ray
{
	vec3 start;
	vec3 dir;
	vec3 inv;
	vec3 col;
	bool terminate;
};

// SSBOs
layout(std430, binding = 0) buffer sceneBuffer
{
	vec3 vertices[100000];
	vec3 normals[100000];
	vec2 textures[100000];
	int indices[100000];
	int vertices_size;
	int normals_size;
	int textures_size;
	int indices_size;
};

layout(std430, binding = 1) buffer bvhBuffer
{
	Node nodes[];
};

// functions
vec3 reflect(vec3 vec, vec3 normal)
{
	vec3 n = normalize(normal);
	float dot = dot(n, vec);
	vec3 temp = n * 2 * dot;
	return vec - temp;
}

vec3 intersect(Ray ray, int tri)
{
	vec3 a = vertices[indices[tri]];
	vec3 b = vertices[indices[tri + 1]];
	vec3 c = vertices[indices[tri + 2]];

	vec3 e1 = b - a;
	vec3 e2 = c - a;

	vec3 ray_cross_e2 = cross(ray.dir, e2);

	float det = dot(e1, ray_cross_e2);

	if (det > -0.0000001 && det < 0.0000001)
		return vec3(0.0);

	float inv_det = 1.0 / det;
	vec3 s = ray.start - a;

	float u = inv_det * dot(s, ray_cross_e2);

	if (u < 0 || u > 1)
		return vec3(0.0);

	vec3 s_cross_e1 = cross(s, e1);

	float v = inv_det * dot(ray.dir, s_cross_e1);

	if (v < 0 || u + v > 1)
		return vec3(0.0);

	float t = inv_det * dot(e2, s_cross_e1);

	return t > 0.00001 ? vec3(t, u, v) : vec3(0.0);
}

bool intersect(Ray ray, Node aabb)
{
	float tx1 = (aabb.min.x - ray.start.x) * ray.inv.x;
	float tx2 = (aabb.max.x - ray.start.x) * ray.inv.x;

	float tmin = min(tx1, tx2);
	float tmax = max(tx1, tx2);

	float ty1 = (aabb.min.y - ray.start.y) * ray.inv.y;
	float ty2 = (aabb.max.y - ray.start.y) * ray.inv.y;

	tmin = max(tmin, min(ty1, ty2));
	tmax = min(tmax, max(ty1, ty2));

	float tz1 = (aabb.min.z - ray.start.z) * ray.inv.z;
	float tz2 = (aabb.max.z - ray.start.z) * ray.inv.z;

	tmin = max(tmin, min(tz1, tz2));
	tmax = min(tmax, max(tz1, tz2));

	return tmax > tmin;
}

Ray traceRay(Ray ray, inout uint seed)
{
	float dist = 999999.9;

	// set default color to skybox color
	vec2 tex_coord = vec2((atan(ray.dir.y, ray.dir.x) + pi / 2.0) / (pi * 2.0), (asin(ray.dir.z) + pi / 2.0) / pi);
	vec3 col = texture(skybox_texture, tex_coord).xyz;// vec3(1.0);// vec3(0.4, 0.8, 1.0);
	vec3 normal = vec3(0.0, 0.0, 1.0);
	bool terminate = true;
	
	// ray traversal
	int to_visit_offset = 0;
	int current_node = 0;
	int nodes_to_visit[32];
	while (true)
	{
		Node node = nodes[current_node];
		if (intersect(ray, node))
		{
			if (node.tri_index > -1) // leaf
			{
				// intersect ray with primitive(s) in leaf node
				for (int i = 0; i < node.num_tris; ++i)
				{
					int tri = (node.tri_index + i) * 3;
					vec3 intersection = intersect(ray, tri);
					if (intersection.x > 0.0 && intersection.x < dist)
					{
						dist = intersection.x;
						float u = intersection.y;
						float v = intersection.z;
						/*vec2 a = textures[indices[tri]];
						vec2 b = textures[indices[tri + 1]];
						vec2 c = textures[indices[tri + 2]];*/
						col = vec3(1.0, 0.82, 0.11); //texture(dragon_texture, (1 - u - v) * a + u * b + v * c).xyz; vec3(0.7, 1.0, 0.2); vec3(1 - intersection.y - intersection.z, intersection.yz);
						normal = cross(vertices[indices[tri + 2]] - vertices[indices[tri]],
							vertices[indices[tri + 1]] - vertices[indices[tri]]);
						//normal = (1 - u - v) * normals[indices[tri]] + (u * normals[indices[tri + 1]]) + (v * normals[indices[tri + 2]]);
						normal = normalize(normal);
						//col = (0.8 * max(dot(normal, vec3(0.0, 0.0, -1.0)), 0.0) + 0.2) * col;
						terminate = false;
					}
				}
				if (to_visit_offset == 0)
					break;
				current_node = nodes_to_visit[--to_visit_offset];
			}
			else // interior
			{
				// put far node on stack, advance to near node
				if (ray.dir[node.axis] < 0)
				{
					nodes_to_visit[to_visit_offset++] = current_node + 1;
					current_node = node.left;
				}
				else
				{
					nodes_to_visit[to_visit_offset++] = node.left;
					current_node = current_node + 1;
				}
			}
		}
		else
		{
			if (to_visit_offset == 0)
				break;
			current_node = nodes_to_visit[--to_visit_offset];
		}
	}

	// update ray
	vec3 dir = normalize(reflect(ray.dir, normal));
	return Ray(ray.start + ray.dir * dist * 0.999, dir, 1.0 / dir, ray.col * col, terminate);
}

void main()
{
	// ray pos and direction calculations
	vec4 ray_start = vec4(0.0, 0.0, 0.0, 1.0);
	vec4 ray_end = vec4((TexCoord.x - 0.5) * (3.0 / 2.0), TexCoord.y - 0.5, -1.0, 1.0);

	vec3 start = (camera * ray_start).xyz;
	vec3 end = (camera * ray_end).xyz;

	// ray creation
	vec3 dir = normalize(end - start);
	Ray ray;
	ray.start = start;
	ray.dir = dir;
	ray.inv = 1.0 / dir;
	ray.col = vec3(1.0);
	ray.terminate = false;

	// ray trace
	uint seed = 0u;
	ray = traceRay(ray, seed);

	FragColor = vec4(ray.col, 1.0);
}