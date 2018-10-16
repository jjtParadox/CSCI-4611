/** CSci-4611 Assignment 3:  Earthquake
 */

#include "earth.h"
#include "config.h"

#include <vector>

// for M_PI constant
#define _USE_MATH_DEFINES
#include <math.h>

std::vector<Point3> plane_vertices;
std::vector<Vector3> plane_normals;
std::vector<Point3> sphere_vertices;
std::vector<Vector3> sphere_normals;

Earth::Earth() {
}

Earth::~Earth() {
}

void Earth::Init(const std::vector<std::string> &search_path) {
    // init shader program
    shader_.Init();
    
    // init texture: you can change to a lower-res texture here if needed
    earth_tex_.InitFromFile(Platform::FindFile("earth-2k.png", search_path));

    // init geometry
    const int nslices = 50;
    const int nstacks = 50;

    std::vector<unsigned int> indices;
    std::vector<Point2> tex_coords;

    for (int i = 0; i <= nslices; i++) {
        for (int j = 0; j <= nstacks; j++) {
            plane_vertices.push_back(Point3((float)((float)i/nslices * 2*M_PI - M_PI), (float)((float)j/nstacks * M_PI - M_PI_2), 0));
            plane_normals.push_back(Vector3(0,0,-1).ToUnit());

            Point3 sphere_pos = LatLongToSphere((float)j/nstacks * 180 - 90, (float)i/nslices * 360 - 180);
            sphere_vertices.push_back(sphere_pos);
            sphere_normals.push_back(sphere_pos - Point3(0,0,0));

            tex_coords.push_back(Point2((float)i/nslices, 1.0f-(float)j/nstacks));
        }
    }

    for (int i = 0; i < nslices; i++) {
        for (int j = 0; j < nstacks; j++) {
            indices.push_back((unsigned int)(i * (nstacks+1) + j));
            indices.push_back((unsigned int)((i+1) * (nstacks+1) + j));
            indices.push_back((unsigned int)((i+1) * (nstacks+1) + (j+1)));

            indices.push_back((unsigned int)(i * (nstacks+1) + j));
            indices.push_back((unsigned int)((i+1) * (nstacks+1) + (j+1)));
            indices.push_back((unsigned int)(i * (nstacks+1) + (j+1)));
        }
    }

    earth_mesh_.SetVertices(plane_vertices);
    earth_mesh_.SetNormals(plane_normals);
    earth_mesh_.SetIndices(indices);
    earth_mesh_.SetTexCoords(0, tex_coords);
    earth_mesh_.UpdateGPUMemory();
}


void Earth::LerpTo(const float target) {
    if (target <= 0.0) {
        earth_mesh_.SetVertices(plane_vertices);
        earth_mesh_.SetNormals(plane_normals);
    } else if (target >= 1.0) {
        earth_mesh_.SetVertices(sphere_vertices);
        earth_mesh_.SetNormals(sphere_normals);
    } else {
        std::vector<Point3> vertices;
        std::vector<Vector3> normals;

        for (int i = 0; i < plane_vertices.size(); i++) {
            Point3 p = plane_vertices[i];
            Vector3 n = plane_normals[i];

            Point3 s_p = sphere_vertices[i];
            Vector3 s_n = sphere_normals[i];

            float p_x = GfxMath::Lerp(p.x(), s_p.x(), target);
            float p_y = GfxMath::Lerp(p.y(), s_p.y(), target);
            float p_z = GfxMath::Lerp(p.z(), s_p.z(), target);

            float n_x = GfxMath::Lerp(n.x(), s_n.x(), target);
            float n_y = GfxMath::Lerp(n.y(), s_n.y(), target);
            float n_z = GfxMath::Lerp(n.z(), s_n.z(), target);

            vertices.push_back(Point3(p_x, p_y, p_z));
            normals.push_back(Vector3(n_x, n_y, n_z));
        }

        earth_mesh_.SetVertices(vertices);
        earth_mesh_.SetNormals(normals);
    }
    earth_mesh_.UpdateGPUMemory();
}


void Earth::Draw(const Matrix4 &model_matrix, const Matrix4 &view_matrix, const Matrix4 &proj_matrix) {
    // Define a really bright white light.  Lighting is a property of the "shader"
    DefaultShader::LightProperties light;
    light.position = Point3(10,10,10);
    light.ambient_intensity = Color(1,1,1);
    light.diffuse_intensity = Color(1,1,1);
    light.specular_intensity = Color(1,1,1);
    shader_.SetLight(0, light);

    // Adust the material properties, material is a property of the thing
    // (e.g., a mesh) that we draw with the shader.  The reflectance properties
    // affect the lighting.  The surface texture is the key for getting the
    // image of the earth to show up.
    DefaultShader::MaterialProperties mat;
    mat.ambient_reflectance = Color(0.5, 0.5, 0.5);
    mat.diffuse_reflectance = Color(0.75, 0.75, 0.75);
    mat.specular_reflectance = Color(0.75, 0.75, 0.75);
    mat.surface_texture = earth_tex_;

    // Draw the earth mesh using these settings
    if (earth_mesh_.num_triangles() > 0) {
        shader_.Draw(model_matrix, view_matrix, proj_matrix, &earth_mesh_, mat);
    }
}


Point3 Earth::LatLongToSphere(double latitude, double longitude) const {
    float latRads = GfxMath::ToRadians((float)latitude);
    float lonRads = GfxMath::ToRadians((float)longitude);
    float x = cosf(latRads) * sinf(lonRads) * (float) M_PI_2;
    float y = sinf(latRads) * (float) M_PI_2;
    float z = cosf(latRads) * cosf(lonRads) * (float) M_PI_2;
    return Point3(x,y,z);
}

Point3 Earth::LatLongToPlane(double latitude, double longitude) const {
    float x = (float) longitude / 180 * (float) M_PI;
    float y = (float) latitude / 90 * (float) M_PI_2;
    return Point3(x,y,0);
}



void Earth::DrawDebugInfo(const Matrix4 &model_matrix, const Matrix4 &view_matrix, const Matrix4 &proj_matrix) {
    // This draws a cylinder for each line segment on each edge of each triangle in your mesh.
    // So it will be very slow if you have a large mesh, but it's quite useful when you are
    // debugging your mesh code, especially if you start with a small mesh.
    for (int t=0; t<earth_mesh_.num_triangles(); t++) {
        std::vector<unsigned int> indices = earth_mesh_.triangle_vertices(t);
        std::vector<Point3> loop;
        loop.push_back(earth_mesh_.vertex(indices[0]));
        loop.push_back(earth_mesh_.vertex(indices[1]));
        loop.push_back(earth_mesh_.vertex(indices[2]));
        quick_shapes_.DrawLines(model_matrix, view_matrix, proj_matrix,
            Color(1,1,0), loop, QuickShapes::LinesType::LINE_LOOP, 0.005);
    }
}

