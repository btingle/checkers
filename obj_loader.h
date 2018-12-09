#pragma once
#include "includes.h"

/*
 * Loads in the vertex data for game objects, like the wall and hatch.
 * Configured to load in data from wavefront .obj files
 */

using namespace std;

bool load_obj(std::vector<float> & out_vertices, const char * filepath)
{
	vector<glm::vec3> temp_norms;
	vector<glm::vec2> temp_uvs;
	vector<glm::vec3> temp_verts;
	vector<unsigned int> norm_indices;
	vector<unsigned int> uv_indices;
	vector<unsigned int> vert_indices;

	FILE * objf = fopen(filepath, "r");
	if (!objf)
	{
		printf("Couldn't open the obj file");
		return false;
	}

	while (true)
	{
		char header[128];
		int result = fscanf(objf, "%s", header);
		if (result == EOF)
		{
			break;
		}
		if (strcmp(header, "v") == 0)
		{
			glm::vec3 vert;
			fscanf(objf, "%f %f %f\n", &vert.x, &vert.y, &vert.z);
			temp_verts.push_back(vert);
		}
		else if (strcmp(header, "vt") == 0)
		{
			glm::vec2 uv;
			fscanf(objf, "%f %f\n", &uv.x, &uv.y);
			uv.y = 1.f - uv.y; // textures coordinates are inverted compared to opengl in the obj format
			temp_uvs.push_back(uv);
		}
		else if (strcmp(header, "vn") == 0)
		{
			glm::vec3 norm;
			fscanf(objf, "%f %f %f\n", &norm.x, &norm.y, &norm.z);
			temp_norms.push_back(norm);
		}
		else if (strcmp(header, "f") == 0)
		{
			unsigned int vert_inds[3], norm_inds[3], uv_inds[3];
			int matches =
				fscanf(objf, "%d/%d/%d %d/%d/%d %d/%d/%d\n", &vert_inds[0], &uv_inds[0], &norm_inds[0],
					&vert_inds[1], &uv_inds[1], &norm_inds[1],
					&vert_inds[2], &uv_inds[2], &norm_inds[2]);
			if (matches != 9)
			{
				printf("File can't be read, try another format");
				return false;
			}
			for (int i = 0; i < 3; i++)
			{
				norm_indices.push_back(norm_inds[i]);
				vert_indices.push_back(vert_inds[i]);
				uv_indices.push_back(uv_inds[i]);
			}
		}
		else
		{
			fscanf(objf, "%*[^\n]\n", NULL); // skips line if header doesn't match any of these formats
		}

	}
	//cout << temp_verts.size() << endl << temp_norms.size() << endl << temp_uvs.size() << endl;
	//cout << vert_indices.size();

	for (int i = 0; i < vert_indices.size(); i++)
	{
		if (vert_indices[i] - 1 >= temp_verts.size())
		{
			cout << "vert indices greater than verts size";
			return false;
		}
		// vertex coordinates
		out_vertices.push_back(temp_verts[vert_indices[i] - 1].x);
		out_vertices.push_back(temp_verts[vert_indices[i] - 1].y);
		out_vertices.push_back(temp_verts[vert_indices[i] - 1].z);

		if (norm_indices[i] - 1 >= temp_norms.size())
		{
			cout << "norm indices greater than norms size";
			return false;
		}

		// vertex normal
		out_vertices.push_back(temp_norms[norm_indices[i] - 1].x);
		out_vertices.push_back(temp_norms[norm_indices[i] - 1].y);
		out_vertices.push_back(temp_norms[norm_indices[i] - 1].z);

		if (uv_indices[i] - 1 >= temp_uvs.size())
		{
			cout << "uv indices greater than uvs size";
			return false;
		}
		// texture coordinates
		out_vertices.push_back(temp_uvs[uv_indices[i] - 1].x);
		out_vertices.push_back(temp_uvs[uv_indices[i] - 1].y);
	}

	return true;
}