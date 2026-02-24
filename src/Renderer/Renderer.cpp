#include "Renderer.h"
#include <glad/glad.h>

void Renderer::drawMeshes(
    const std::vector<Mesh>& meshes,
    std::unordered_map<int, unsigned int>& textures,
    Shader& shader)
{
    for (auto& mesh : meshes)
    {
        if (mesh.materialID >= 0 && textures.count(mesh.materialID))
        {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textures[mesh.materialID]);
            shader.setInt("texture1", 0);
        }

        glBindVertexArray(mesh.vao);
        glDrawArrays(GL_TRIANGLES, 0, mesh.vertexCount);
    }
}