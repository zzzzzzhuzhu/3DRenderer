#ifndef SKYBOX_H
#define SKYBOX_H

#include "texture.h"
#include "shaderprogram.h"
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QMatrix4x4>
#include <memory>

/// Class for the skybox
/**
 * @brief Defines a skybox.
 * @author Louis Filipozzi
 */
class Skybox {
public:
    Skybox();
    ~Skybox();
    
    /**
     * @brief Initialize the skybox, i.e. create the buffers, attributes, ...
     */
    void initialize();
    
    /**
     * @brief Draw the skybox
     * @param view The view matrix.
     * @param projection The projection matrix.
     * @param shader The shader program used to render the skybox.
     * @param glFunctions Pointer to class containing OpenGL functions.
     */
    void render(const QMatrix4x4 & view, const QMatrix4x4 & projection, 
                QOpenGLFunctions_3_3_Core * glFunctions);
    
    /**
     * @brief Clean up the skybox.
     */
    void cleanUp();
    
private:
    /**
     * @brief Create and link the shader program.
     * @param vShader The path to the source file of the vertex shader.
     * @param fShader The path to the source file of the fragment shader.
     */
    void createShaderProgram(QString vShader, QString fShader);
    
    /**
     * @brief Load the cubemap textures.
     */
    void loadCubeMapTextures();
    
    /**
     * @brief Set attributes of the shaders program.
     */
    void createAttributes();

    /**
     * @brief Create the vertex, normal, texture, and index buffers.
     */
    void createBuffers();
    
    /**
     * @brief Textures used by the skybox.
     */
    std::unique_ptr<Texture> m_textures;
    
    /**
     * OpenGL Vertex Array Object (VAO)
     */
    QOpenGLVertexArrayObject m_vao;
    
    /**
     * Buffer of vertices
     */
    QOpenGLBuffer m_vertexBuffer;
    
    /**
     * Shader program used to render the skybox.
     */
    std::unique_ptr<Shader> m_shader;
    
};

#endif // SKYBOX_H
